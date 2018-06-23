#include "rat.h"

Buffer *new_buffer(pid_t pid, int fd) {
  Buffer *b = (Buffer*)malloc(sizeof(Buffer));

  b->pid = pid;
  b->fd = fd;
  b->stream = new_stream();
  b->line_ends = new_line_ends();
  b->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  pthread_create(&b->fill_thread, NULL, fill_buffer, b);

  return b;
}

void free_buffer(Buffer *b) {
  free_stream(b->stream);
  free_line_ends(b->line_ends);
  free(b);
}

void *fill_buffer(void *bptr) {
  Buffer *b = (Buffer*)bptr;
  ssize_t n;
  Token t;
  Tokenizer *tr = new_tokenizer(b->fd);

  while (1) {
    t.value = new_strbuf("");
    n = read_token(tr, &t);

    switch (t.type) {
    case TK_NONE:
      break;
    case TK_NEWLINE:
      stream_write(b->stream, t.value->str);
      break;
    case TK_CONTENT:
      stream_write(b->stream, t.value->str);
      break;
    case TK_TERMSTYLE:
      break;
    }

    free_strbuf(t.value);

    if (n <= 0) {
      break;
    }
  }

  if (n == -1) {
    perror("fill_buffer");
    exit(EXIT_FAILURE);
  }

  free_tokenizer(tr);

  waitpid(b->pid, NULL, 0);
}

