#include "annotate.h"

Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value) {
  Annotation *a = malloc(sizeof(*a));

  a->start = start;
  a->end = end;
  a->type = type;
  a->value = value;

  return a;
}

void free_annotation(Annotation *a) {
  // TODO: Will need to free strings once they're dynamically allocated
  // free(a->type);
  // free(a->value);
  free(a);
}

Annotator *new_annotator(Buffer *b, char *cmd) {
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

      execl(shell, shell, "-c", cmd, NULL);
      perror("execl");
      exit(EXIT_FAILURE);
  }

  close(content_fds[0]);
  close(annotation_fds[1]);

  // Don't block on reads
  if ((flags = fcntl(annotation_fds[0], F_GETFL, 0)) < 0) {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
  fcntl(annotation_fds[0], F_SETFL, flags | O_NONBLOCK);

  a = malloc(sizeof(*a));

  a->buffer = b;
  a->pid = pid;
  a->wfd = content_fds[1];
  a->rfd = annotation_fds[0];
  a->woffset = 0;

  return a;
}

void annotator_write(Annotator *a) {
  int n = a->buffer->stream_len - a->woffset;
  int written;

  if (n > 0) {
    written = write(a->wfd, a->buffer->stream_str + a->woffset, n);

    if (written == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "wrote %d bytes to annotator\n", written);

    a->woffset += written;
  }
}

Annotation *annotator_read(Annotator *a) {
  ssize_t n;
  char buf[33];

  while ((n = read(a->rfd, buf, 32)) > 0) {
    fprintf(stderr, "read %lu bytes from annotator\n", n);

    // TODO: Store in temp buffer to handle reading partial annotations
  }

  if (n < 0 && errno != EAGAIN) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  return new_annotation(5, 10, "foo.bar", "foo bar baz");
}

void kill_annotator(Annotator *a) {
  if (kill(a->pid, SIGTERM) < 0) {
    perror("kill");
    exit(EXIT_FAILURE);
  }

  waitpid(a->pid, NULL, 0);
}

void free_annotator(Annotator *a) {
  close(a->wfd);
  close(a->rfd);

  free(a);
}
