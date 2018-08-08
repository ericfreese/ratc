#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/poll.h>

#include "annotation_parser.h"
#include "io.h"
#include "tokenizer.h"

static const size_t ANNOTATOR_READ_LEN = 4096;
static const size_t BUFFER_READ_LEN = 32768;

typedef struct buffer_io BufferIO;
struct buffer_io {
  BufferIO *next;

  Buffer *buffer;
  Tokenizer *tokenizer;

  pid_t pid;

  int rfd;
  int rdone;
};

typedef struct annotator_io AnnotatorIO;
struct annotator_io {
  AnnotatorIO *next;

  Buffer *buffer;
  AnnotationParser *parser;

  pid_t pid;

  int wfd;
  size_t woffset;
  int wdone;

  int rfd;
  int rdone;
};

typedef struct {
  int ttyin;
  void (*tty_handler)(int ch);
  BufferIO *buffers;
  AnnotatorIO *annotators;
  size_t n_items;
} IOManager;

IOManager *io;

int io_buffer_is_running(Buffer *b) {
  for (BufferIO *bio = io->buffers; bio != NULL; bio = bio->next) {
    if (bio->buffer == b) {
      return !bio->rdone;
    }
  }

  return 0;
}

void io_start_buffer(Buffer *b, const char *cmd) {
  int fds[2];
  char* shell;
  pid_t pid;

  pipe(fds);

  switch (pid = fork()) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      shell = getenv("SHELL");

      dup2(fds[1], STDOUT_FILENO);
      dup2(fds[1], STDERR_FILENO);

      close(fds[0]);
      close(fds[1]);

      setpgrp();

      execl(shell, shell, "-c", cmd, NULL);
      perror("execl");
      exit(EXIT_FAILURE);
  }

  close(fds[1]);

  BufferIO *bio = malloc(sizeof *bio);

  bio->buffer = b;
  bio->tokenizer = new_tokenizer();
  bio->pid = pid;
  bio->rfd = fds[0];
  bio->rdone = 0;

  bio->next = io->buffers;
  io->buffers = bio;
  io->n_items++;
}

void free_buffer_io(BufferIO *bio) {
  free_tokenizer(bio->tokenizer);
  free(bio);
}

ssize_t buffer_io_read(BufferIO *bio) {
  char *buf = malloc(BUFFER_READ_LEN);
  ssize_t n;

  if ((n = read(bio->rfd, buf, BUFFER_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //fprintf(stderr, "read %ld bytes in buffer_io_read\n", n);

  if (n) {
    Token *t;

    tokenizer_write(bio->tokenizer, buf, n);

    while ((t = tokenizer_read(bio->tokenizer)) != NULL) {
      buffer_handle_token(bio->buffer, t);
      free_token(t);
    }
  } else {
    bio->rdone = 1;
  }

  free(buf);
  return n;
}

void io_start_annotating_buffer(Buffer *b, Annotator *ar) {
  char* shell;
  pid_t pid;

  int content_fds[2];
  int annotation_fds[2];
  int flags;

  pipe(content_fds);
  pipe(annotation_fds);

  // TODO: Can we move this functionality to annotator_fork(ar) and have it
  // return a struct with pid, rfd, wfd, etc? Then we could remove the
  // annotator_command and annotator_type getters
  switch ((pid = fork())) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      shell = getenv("SHELL");

      dup2(content_fds[0], STDIN_FILENO);
      close(content_fds[0]);
      close(content_fds[1]);

      dup2(annotation_fds[1], STDOUT_FILENO);
      close(annotation_fds[0]);
      close(annotation_fds[1]);

      setpgrp();

      execl(shell, shell, "-c", annotator_command(ar), NULL);
      perror("execl");
      exit(EXIT_FAILURE);
  }

  close(content_fds[0]);
  close(annotation_fds[1]);

  /* Don't block on writes */
  if ((flags = fcntl(content_fds[1], F_GETFL, 0)) < 0) {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
  fcntl(content_fds[1], F_SETFL, flags | O_NONBLOCK);

  //fprintf(stderr, "forked annotator io pid: %d\n", pid);
  AnnotatorIO *aio = malloc(sizeof *aio);

  aio->buffer = b;
  aio->parser = new_annotation_parser(annotator_type(ar));
  aio->pid = pid;
  aio->wfd = content_fds[1];
  aio->woffset = 0;
  aio->wdone = 0;
  aio->rfd = annotation_fds[0];
  aio->rdone = 0;

  aio->next = io->annotators;
  io->annotators = aio;
  io->n_items++;
}

void free_annotator_io(AnnotatorIO *aio) {
  free_annotation_parser(aio->parser);
  free(aio);
}

ssize_t annotator_io_write(AnnotatorIO *aio) {
  int remaining = buffer_len(aio->buffer) - aio->woffset;
  int n;

  if (remaining > 0) {
    n = write(aio->wfd, buffer_content(aio->buffer, aio->woffset), remaining);

    if (n >= 0) {
      //fprintf(stderr, "wrote %d bytes to annotator %d\n", n, aio->wfd);
      aio->woffset += n;
    } else if (errno == EAGAIN) {
      //fprintf(stderr, "could not write %d bytes to annotator %d\n", n, aio->wfd);
    } else {
      perror("write");
      exit(EXIT_FAILURE);
    }
  }

  if (aio->woffset == buffer_len(aio->buffer) && !io_buffer_is_running(aio->buffer)) {
    aio->wdone = 1;
    close(aio->wfd);
  }

  return n;
}

ssize_t annotator_io_read(AnnotatorIO *aio) {
  char *buf = malloc(ANNOTATOR_READ_LEN);
  ssize_t n;

  if ((n = read(aio->rfd, buf, ANNOTATOR_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  //fprintf(stderr, "read %ld bytes in annotator_read\n", n);

  if (n) {
    Annotation *a;

    annotation_parser_write(aio->parser, buf, n);

    while ((a = annotation_parser_read(aio->parser)) != NULL) {
      buffer_add_annotation(aio->buffer, a);
      annotation_ref_dec(a);
    }
  } else {
    aio->rdone = 1;
  }

  free(buf);
  return n;
}

typedef enum {
  IO_TTY_IN = 0,
  IO_BUFFER_IN,
  IO_ANNOTATOR_OUT,
  IO_ANNOTATOR_IN
} IOItemType;

typedef struct io_item IOItem;
struct io_item {
  IOItem *next;
  IOItemType type;
  void *ptr;
  int fd;
};

typedef struct {
  size_t len;
  IOItem **items;
} IOItems;

IOItems *new_io_items() {
  IOItems *ios;

  if ((ios = malloc(sizeof *ios)) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  ios->len = 0;

  if ((ios->items = malloc((io->n_items + 1) * sizeof *ios)) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  return ios;
}

void io_items_add(IOItems *ios, IOItem *item) {
  if (ios->len >= io->n_items + 1) {
    return;
  }

  ios->items[ios->len] = item;
  ios->len++;
}

void free_poll_items(IOItems *ios) {
  free(ios->items);
  free(ios);
}

void io_init(void (*tty_handler)(int ch)) {
  io = malloc(sizeof *io);

  io->ttyin = open("/dev/tty", O_RDONLY);
  io->tty_handler = tty_handler;
  io->buffers = NULL;
  io->annotators = NULL;
  io->n_items = 0;
}

void io_cleanup() {
  close(io->ttyin);

  // TODO: Free io->buffers
  // TODO: Free io->annotators

  free(io);
}

void io_close_buffer(Buffer *b) {
  for (BufferIO *bio = io->buffers; bio != NULL; bio = bio->next) {
    if (bio->buffer == b) {
      if (!bio->rdone) {
        if (kill(-bio->pid, SIGTERM) == -1) {
          perror("kill");
          exit(EXIT_FAILURE);
        }

        bio->rdone = 1;
      }

      break;
    }
  }

  for (AnnotatorIO *aio = io->annotators; aio != NULL; aio = aio->next) {
    if (aio->buffer == b) {
      if (!aio->rdone) {
        if (kill(-aio->pid, SIGTERM) == -1) {
          perror("kill");
          exit(EXIT_FAILURE);
        }

        aio->wdone = 1;
        aio->rdone = 1;
      }

      break;
    }
  }
};

IOItem *new_io_item(IOItemType type, void *ptr, int fd) {
  IOItem *item;

  if ((item = malloc(sizeof *item)) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  item->type = type;
  item->ptr = ptr;
  item->fd = fd;

  return item;
}

IOItems *build_io_items() {
  IOItems *ios = new_io_items();

  io_items_add(ios, new_io_item(IO_TTY_IN, NULL, io->ttyin));

  for (BufferIO *bio = io->buffers; bio != NULL; bio = bio->next) {
    if (!bio->rdone) {
      io_items_add(ios, new_io_item(IO_BUFFER_IN, bio, bio->rfd));
    }
  }

  for (AnnotatorIO *aio = io->annotators; aio != NULL; aio = aio->next) {
    if (!aio->wdone && (aio->woffset < buffer_len(aio->buffer) || !io_buffer_is_running(aio->buffer))) {
      io_items_add(ios, new_io_item(IO_ANNOTATOR_OUT, aio, aio->wfd));
    }

    if (!aio->rdone) {
      io_items_add(ios, new_io_item(IO_ANNOTATOR_IN, aio, aio->rfd));
    }
  }

  return ios;
}

void free_io_items(IOItems *ios) {
  for (size_t i = 0; i < ios->len; i++) {
    free(ios->items[i]);
  }

  free(ios->items);
  free(ios);
}

struct pollfd *build_pollfd(IOItems *ios) {
  struct pollfd *pfd;

  if ((pfd = malloc(ios->len * sizeof *pfd)) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < ios->len; i++) {
    pfd[i].fd = ios->items[i]->fd;

    switch (ios->items[i]->type) {
      case IO_TTY_IN:
      case IO_BUFFER_IN:
      case IO_ANNOTATOR_IN:
        pfd[i].events = POLLIN;
        break;
      case IO_ANNOTATOR_OUT:
        pfd[i].events = POLLOUT;
        break;
    }
  }

  return pfd;
}

void io_prune_finished() {
  BufferIO *pbio = NULL;
  BufferIO *nbio = NULL;
  for (BufferIO *bio = io->buffers; bio != NULL; bio = nbio) {
    nbio = bio->next;

    if (bio->rdone) {
      close(bio->rfd);
      waitpid(bio->pid, NULL, 0);

      if (pbio == NULL) {
        io->buffers = bio->next;
      } else {
        pbio->next = bio->next;
      }

      free_buffer_io(bio);
    }

    pbio = bio;
  }

  AnnotatorIO *paio = NULL;
  AnnotatorIO *naio = NULL;
  for (AnnotatorIO *aio = io->annotators; aio != NULL; aio = naio) {
    naio = aio->next;

    if (aio->rdone) {
      close(aio->rfd);
      waitpid(aio->pid, NULL, 0);

      if (paio == NULL) {
        io->annotators = aio->next;
      } else {
        paio->next = aio->next;
      }

      free_annotator_io(aio);
    }
  }
}

void io_tick() {
  int n;
  size_t i;

  IOItems *ios;
  struct pollfd *pfd;

  ios = build_io_items();
  pfd = build_pollfd(ios);

start_poll:
  n = poll(pfd, ios->len, -1);

  //fprintf(stderr, "poll returned %d\n", n);

  if (n == -1) {
    if (errno == EINTR) {
      goto start_poll;
    }

    perror("poll");
    exit(EXIT_FAILURE);
  }


  /* The first poll fd is the terminal. If user input is present, handle it by
   * itself so we get a fresh poll result
   */
  if (pfd[0].revents & POLLIN) {
    //fprintf(stderr, "got tty input\n");
    io->tty_handler(getch());
  } else {
    for (i = 1; i < ios->len; i++) {
      switch (ios->items[i]->type) {
        case IO_BUFFER_IN:
          if (pfd[i].revents & POLLHUP) {
            //fprintf(stderr, "POLLHUP ready for buffer %ld\n", i);
            while (buffer_io_read(ios->items[i]->ptr));
          } else if (pfd[i].revents & POLLIN) {
            //fprintf(stderr, "POLLIN ready for buffer %ld\n", i);
            buffer_io_read(ios->items[i]->ptr);
          }
          break;

        case IO_ANNOTATOR_OUT:
          if (pfd[i].revents & POLLOUT) {
            //fprintf(stderr, "POLLOUT ready for annotator write %ld\n", i);
            annotator_io_write(ios->items[i]->ptr);
          }
          break;

        case IO_ANNOTATOR_IN:
          if (pfd[i].revents & POLLHUP) {
            //fprintf(stderr, "POLLHUP ready for annotator read %ld\n", i);
            while (annotator_io_read(ios->items[i]->ptr));
          } else if (pfd[i].revents & POLLIN) {
            //fprintf(stderr, "POLLIN ready for annotator read %ld\n", i);
            annotator_io_read(ios->items[i]->ptr);
          }
          break;

        case IO_TTY_IN:
          /* shouldn't get here */
          break;
      }
    }
  }

  io_prune_finished();

  free_io_items(ios);
  free(pfd);
}
