#include "buffer.h"

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

    pthread_mutex_lock(&b->lock);

    switch (t.type) {
    case TK_NONE:
      break;
    case TK_NEWLINE:
      stream_write(b->stream, t.value->str);
      push_line_end(b->line_ends, b->stream->strbuf->len);
      break;
    case TK_CONTENT:
      stream_write(b->stream, t.value->str);
      break;
    case TK_TERMSTYLE:
      break;
    }

    pthread_mutex_unlock(&b->lock);

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

char **get_buffer_lines(Buffer *b, size_t start, size_t num) {
  if (start >= b->line_ends->len) {
    num = 0;
  } else if (start + num > b->line_ends->len) {
    num = b->line_ends->len - start;
  }

  char **buffer_lines = (char**)malloc((num + 1) * sizeof(*buffer_lines));
  size_t start_offset = 0;
  size_t line_len;

  for (size_t i = 0, *next_offset = b->line_ends->offsets; i < num; i++, next_offset++) {
    line_len = *next_offset - start_offset;

    buffer_lines[i] = (char*)malloc((line_len + 1) * sizeof(*buffer_lines[i]));
    memcpy(buffer_lines[i], b->stream->strbuf->str + start_offset, line_len);
    buffer_lines[i][line_len] = '\0';

    start_offset = *next_offset;
  }

  buffer_lines[num] = NULL;

  return buffer_lines;
}
