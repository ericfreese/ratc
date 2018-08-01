#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "buffer.h"
#include "poll_registry.h"
#include "util.h"
#include "refs.h"

static const size_t ANNOTATOR_READ_LEN = 4096;

struct annotator {
  const char* cmd;
  const char* annotation_type;
  struct refs refs;
};

void free_annotator(const struct refs *r) {
  Annotator *ar = container_of(r, Annotator, refs);

  fprintf(stderr, "FREEING ANNOTATOR %p\n", ar);

  free((char*)ar->cmd);
  free((char*)ar->annotation_type);
  free(ar);
}

Annotator *new_annotator(char *cmd, char *annotation_type) {
  Annotator *ar = malloc(sizeof(*ar));

  ar->cmd = strdup(cmd);
  ar->annotation_type = strdup(annotation_type);
  ar->refs = (struct refs){free_annotator, 1};

  return ar;
}

void annotator_ref_inc(Annotator *ar) {
  ref_inc(&ar->refs);
}

void annotator_ref_dec(Annotator *ar) {
  ref_dec(&ar->refs);
}


struct annotator_process {
  Buffer *buffer;
  AnnotationParser *parser;
  size_t woffset;

  pid_t pid;
  int wfd;
  int rfd;
};

AnnotatorProcess *new_annotator_process(Annotator *ar, Buffer *b) {
  char* shell;
  pid_t pid;

  int content_fds[2];
  int annotation_fds[2];
  int flags;

  AnnotatorProcess *ap;

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

      execl(shell, shell, "-c", ar->cmd, NULL);
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

  ap = malloc(sizeof(*ap));

  ap->buffer = b;
  ap->parser = new_annotation_parser(strdup(ar->annotation_type));
  ap->pid = pid;
  ap->wfd = content_fds[1];
  ap->rfd = annotation_fds[0];
  ap->woffset = 0;

  poll_registry_add(PI_ANNOTATOR_WRITE, ap, ap->wfd);
  poll_registry_add(PI_ANNOTATOR_READ, ap, ap->rfd);

  return ap;
}

void annotator_process_write(AnnotatorProcess *ap) {
  int n = buffer_len(ap->buffer) - ap->woffset;
  int written;

  if (n > 0) {
    written = write(ap->wfd, buffer_content(ap->buffer, ap->woffset), n);

    if (written >= 0) {
      fprintf(stderr, "wrote %d bytes to annotator %d\n", written, ap->wfd);
      ap->woffset += written;
    } else if (errno == EAGAIN) {
      fprintf(stderr, "could not write %d bytes to annotator %d\n", n, ap->wfd);
    } else {
      perror("write");
      exit(EXIT_FAILURE);
    }
  }

  if (ap->woffset == buffer_len(ap->buffer) && !buffer_is_running(ap->buffer)) {
    close(ap->wfd);
    poll_registry_remove(ap->wfd);
  }
}

int annotator_process_should_poll_read(AnnotatorProcess *ap) {
  return ap->woffset < buffer_len(ap->buffer) || !buffer_is_running(ap->buffer);
}

ssize_t annotator_process_read(AnnotatorProcess *ap) {
  char *buf = malloc(ANNOTATOR_READ_LEN);
  ssize_t n;

  Annotation *a;

  if ((n = read(ap->rfd, buf, ANNOTATOR_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "read %ld bytes in annotator_read\n", n);

  if (n) {
    annotation_parser_write(ap->parser, buf, n);

    while ((a = annotation_parser_read(ap->parser)) != NULL) {
      buffer_add_annotation(ap->buffer, a);
      annotation_ref_dec(a);
    }
  } else {
    waitpid(ap->pid, NULL, 0);
    close(ap->rfd);
    poll_registry_remove(ap->rfd);
    free_annotator_process(ap);
  }

  return n;
}

void annotator_process_read_all(AnnotatorProcess *ap) {
  while (annotator_process_read(ap));
}

void kill_annotator_process(AnnotatorProcess *ap) {
  if (kill(ap->pid, SIGTERM) < 0) {
    perror("kill");
    exit(EXIT_FAILURE);
  }

  waitpid(ap->pid, NULL, 0);
}

void free_annotator_process(AnnotatorProcess *ap) {
  free(ap);
}
