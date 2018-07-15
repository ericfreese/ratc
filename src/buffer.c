#include "buffer.h"
#include "poll_registry.h"

static const size_t BUFFER_READ_LEN = 32768;

Buffer *new_buffer(pid_t pid, int fd) {
  Buffer *b = (Buffer*)malloc(sizeof(Buffer));

  b->pid = pid;
  b->fd = fd;
  b->stream = open_memstream(&b->stream_str, &b->stream_len);
  b->tr = new_tokenizer(b->fd);
  b->line_ends = new_line_ends();

  poll_registry_add(PI_BUFFER_READ, b, b->fd);

  return b;
}

void free_buffer(Buffer *b) {
  fclose(b->stream);
  free(b->stream_str);
  free_line_ends(b->line_ends);
  free_tokenizer(b->tr);
  free(b);
}

ssize_t buffer_read(Buffer *b) {
  char buf[BUFFER_READ_LEN];
  ssize_t n;
  Token *t;

  if ((n = read(b->fd, buf, BUFFER_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "read %ld bytes in buffer_read\n", n);

  if (n) {
    tokenizer_buffer_input(b->tr, buf, n);

    while ((t = read_token(b->tr)) != NULL) {
      switch (t->type) {
      case TK_NEWLINE:
        fputs(t->value, b->stream);
        fflush(b->stream);
        push_line_end(b->line_ends, b->stream_len);
        break;
      case TK_CONTENT:
        fputs(t->value, b->stream);
        fflush(b->stream);
        break;
      case TK_TERMSTYLE:
        // TODO
        break;
      case TK_NONE:
        break;
      }

      free_token(t);
    }
  } else {
    waitpid(b->pid, NULL, 0);
    poll_registry_remove(b->fd);
  }

  return n;
}

void buffer_read_all(Buffer *b) {
  while (buffer_read(b));
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
    memcpy(buffer_lines[i], b->stream_str + start_offset, line_len);
    buffer_lines[i][line_len] = '\0';

    start_offset = *next_offset;
  }

  buffer_lines[num] = NULL;

  return buffer_lines;
}
