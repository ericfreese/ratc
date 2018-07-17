#include <unistd.h>
#include "annotate.h"
#include "poll_registry.h"
#include "str_util.h"

static const size_t ANNOTATOR_READ_LEN = 4096;

Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value) {
  Annotation *a = malloc(sizeof(*a));

  a->start = start;
  a->end = end;
  a->type = type;
  a->value = value;

  return a;
}

void free_annotation(Annotation *a) {
  free(a->type);
  free(a->value);
  free(a);
}

AnnotationParser *new_annotation_parser(char *annotation_type) {
  AnnotationParser *ap = malloc(sizeof(*ap));

  ap->version = 0;
  ap->has_version = 0;
  ap->rq = new_read_queue();
  ap->annotation_type = annotation_type;

  return ap;
}

void free_annotation_parser(AnnotationParser *ap) {
  free_read_queue(ap->rq);
  free(ap->annotation_type);
  free(ap);
}

void annotation_parser_buffer_input(AnnotationParser *ap, char *buf, size_t len) {
  read_queue_write(ap->rq, buf, len);
}

Annotation *read_annotation(AnnotationParser *ap) {
  size_t n;
  unsigned char version;
  uint64_t num[3];
  char *val;

  if (!ap->has_version) {
    if ((n = read_queue_read(ap->rq, &version, 1)) < 1) {
      goto not_enough_input;
    };

    if (version != 1) {
      fprintf(stderr, "invalid annotator version: %d\n", version);
      exit(EXIT_FAILURE);
    }

    ap->version = version;
    ap->has_version = 1;
  }

  if ((n = read_queue_read(ap->rq, &num, 3 * sizeof(*num))) < 3 * sizeof(*num)) {
    goto not_enough_input;
  };

  val = malloc(num[2] * sizeof(*val) + 1);

  if ((n = read_queue_read(ap->rq, val, num[2])) < num[2]) {
    free(val);
    goto not_enough_input;
  }

  val[n] = '\0';

  read_queue_commit(ap->rq);

  return new_annotation(num[0], num[1], copy_string(ap->annotation_type), val);

not_enough_input:
  fprintf(stderr, "NOT ENOUGH INPUT\n");
  read_queue_rollback(ap->rq);
  return NULL;
}

Annotator *new_annotator(Buffer *b, char *cmd, char *annotation_type) {
  char* shell;
  pid_t pid;

  int content_fds[2];
  int annotation_fds[2];
  int flags;

  Annotator *a;

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

  // Don't block on writes
  if ((flags = fcntl(content_fds[1], F_GETFL, 0)) < 0) {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
  fcntl(content_fds[1], F_SETFL, flags | O_NONBLOCK);

  //// Don't block on reads
  //if ((flags = fcntl(annotation_fds[0], F_GETFL, 0)) < 0) {
  //  perror("fcntl");
  //  exit(EXIT_FAILURE);
  //}
  //fcntl(annotation_fds[0], F_SETFL, flags | O_NONBLOCK);

  a = malloc(sizeof(*a));

  a->buffer = b;
  a->parser = new_annotation_parser(copy_string(annotation_type));
  a->pid = pid;
  a->wfd = content_fds[1];
  a->rfd = annotation_fds[0];
  a->woffset = 0;

  poll_registry_add(PI_ANNOTATOR_WRITE, a, a->wfd);
  poll_registry_add(PI_ANNOTATOR_READ, a, a->rfd);

  return a;
}

void annotator_write(Annotator *a) {
  int n = a->buffer->stream_len - a->woffset;
  int written;

  if (n > 0) {
    written = write(a->wfd, a->buffer->stream_str + a->woffset, n);

    if (written >= 0) {
      fprintf(stderr, "wrote %d bytes to annotator %d\n", written, a->wfd);
      a->woffset += written;
    } else if (errno == EAGAIN) {
      fprintf(stderr, "could not write %d bytes to annotator %d\n", n, a->wfd);
    } else {
      perror("write");
      exit(EXIT_FAILURE);
    }
  }

  if (a->woffset == a->buffer->stream_len && !a->buffer->is_running) {
    close(a->wfd);
    poll_registry_remove(a->wfd);
  }
}

ssize_t annotator_read(Annotator *a) {
  char *buf = malloc(ANNOTATOR_READ_LEN);
  ssize_t n;

  Annotation *t;

  if ((n = read(a->rfd, buf, ANNOTATOR_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "read %ld bytes in annotator_read\n", n);

  if (n) {
    annotation_parser_buffer_input(a->parser, buf, n);

    while ((t = read_annotation(a->parser)) != NULL) {
      // TODO: Add the annotation to the buffer!
      fprintf(stderr, "got annotation: %s: @ %d - %d / %s\n", t->type, t->start, t->end, t->value);

      free_annotation(t);
    }
  } else {
    waitpid(a->pid, NULL, 0);
    close(a->rfd);
    poll_registry_remove(a->rfd);
    free_annotator(a);
  }

  return n;
}

void annotator_read_all(Annotator *a) {
  while (annotator_read(a));
}

void kill_annotator(Annotator *a) {
  if (kill(a->pid, SIGTERM) < 0) {
    perror("kill");
    exit(EXIT_FAILURE);
  }

  waitpid(a->pid, NULL, 0);
}

void free_annotator(Annotator *a) {
  free(a);
}
