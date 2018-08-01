#include <errno.h>
#include <fcntl.h>

#include "buffer.h"
#include "poll_registry.h"
#include "util.h"

static const size_t ANNOTATOR_READ_LEN = 4096;

struct annotator {
  Buffer *buffer;
  AnnotationParser *parser;
  size_t woffset;

  pid_t pid;
  int wfd;
  int rfd;
};

Annotator *new_annotator(Buffer *b, char *cmd, char *annotation_type) {
  char* shell;
  pid_t pid;

  int content_fds[2];
  int annotation_fds[2];
  int flags;

  Annotator *ar;

  pipe(content_fds);
  pipe(annotation_fds);

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

      execl(shell, shell, "-c", cmd, NULL);
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

  ar = malloc(sizeof(*ar));

  ar->buffer = b;
  ar->parser = new_annotation_parser(copy_string(annotation_type));
  ar->pid = pid;
  ar->wfd = content_fds[1];
  ar->rfd = annotation_fds[0];
  ar->woffset = 0;

  poll_registry_add(PI_ANNOTATOR_WRITE, ar, ar->wfd);
  poll_registry_add(PI_ANNOTATOR_READ, ar, ar->rfd);

  return ar;
}

void annotator_write(Annotator *ar) {
  int n = buffer_len(ar->buffer) - ar->woffset;
  int written;

  if (n > 0) {
    written = write(ar->wfd, buffer_content(ar->buffer, ar->woffset), n);

    if (written >= 0) {
      fprintf(stderr, "wrote %d bytes to annotator %d\n", written, ar->wfd);
      ar->woffset += written;
    } else if (errno == EAGAIN) {
      fprintf(stderr, "could not write %d bytes to annotator %d\n", n, ar->wfd);
    } else {
      perror("write");
      exit(EXIT_FAILURE);
    }
  }

  if (ar->woffset == buffer_len(ar->buffer) && !buffer_is_running(ar->buffer)) {
    close(ar->wfd);
    poll_registry_remove(ar->wfd);
  }
}

ssize_t annotator_read(Annotator *ar) {
  char *buf = malloc(ANNOTATOR_READ_LEN);
  ssize_t n;

  Annotation *a;

  if ((n = read(ar->rfd, buf, ANNOTATOR_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "read %ld bytes in annotator_read\n", n);

  if (n) {
    annotation_parser_write(ar->parser, buf, n);

    while ((a = annotation_parser_read(ar->parser)) != NULL) {
      buffer_add_annotation(ar->buffer, a);
      annotation_ref_dec(a);
    }
  } else {
    waitpid(ar->pid, NULL, 0);
    close(ar->rfd);
    poll_registry_remove(ar->rfd);
    free_annotator(ar);
  }

  return n;
}

void annotator_read_all(Annotator *ar) {
  while (annotator_read(ar));
}

void kill_annotator(Annotator *ar) {
  if (kill(ar->pid, SIGTERM) < 0) {
    perror("kill");
    exit(EXIT_FAILURE);
  }

  waitpid(ar->pid, NULL, 0);
}

void free_annotator(Annotator *ar) {
  free(ar);
}
