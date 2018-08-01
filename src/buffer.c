#include <signal.h>

#include "buffer.h"
#include "poll_registry.h"

static const size_t BUFFER_READ_LEN = 32768;

typedef struct line_ends LineEnds;
struct line_ends {
  size_t *offsets;
  size_t len;
  size_t size;
};

LineEnds *new_line_ends() {
  LineEnds *le = (LineEnds*)malloc(sizeof(*le));

  le->len = 0;
  le->size = 8;
  le->offsets = (size_t*)malloc(le->size * sizeof(*le->offsets));

  return le;
}

void free_line_ends(LineEnds *le) {
  free(le->offsets);
  free(le);
}

void push_line_end(LineEnds *le, size_t offset) {
  if (le->len == le->size) {
    le->size *= 2;
    le->offsets = (size_t*)realloc(le->offsets, le->size * sizeof(*le->offsets));
  }

  le->offsets[le->len] = offset;

  le->len++;
}

struct buffer {
  pid_t pid;
  int fd;
  int is_running;

  char *stream_str;
  size_t stream_len;
  FILE *stream;

  LineEnds *line_ends;
  Tokenizer *tokenizer;
  Annotations *annotations;
};

Buffer *new_buffer(pid_t pid, int fd) {
  Buffer *b = (Buffer*)malloc(sizeof(Buffer));

  b->pid = pid;
  b->fd = fd;
  b->stream_len = 0;
  b->stream = open_memstream(&b->stream_str, &b->stream_len);
  b->tokenizer = new_tokenizer(b->fd);
  b->line_ends = new_line_ends();
  b->is_running = 1;
  b->annotations = new_annotations();

  poll_registry_add(PI_BUFFER_READ, b, b->fd);

  return b;
}

void free_buffer(Buffer *b) {
  free_annotations(b->annotations);
  free(b->stream_str);
  free_line_ends(b->line_ends);
  free_tokenizer(b->tokenizer);
  free(b);
}

void cleanup_buffer(Buffer *b) {
  b->is_running = 0;
  waitpid(b->pid, NULL, 0);
  close(b->fd);
  fclose(b->stream);
  poll_registry_remove(b->fd);
}

void close_buffer(Buffer *b) {
  if (b->is_running) {
    kill(-b->pid, SIGTERM);
    cleanup_buffer(b);
  }
}

ssize_t buffer_read(Buffer *b) {
  char *buf = malloc(BUFFER_READ_LEN);
  ssize_t n;
  Token *t;

  if ((n = read(b->fd, buf, BUFFER_READ_LEN)) == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "read %ld bytes in buffer_read\n", n);

  if (n) {
    tokenizer_write(b->tokenizer, buf, n);

    while ((t = tokenizer_read(b->tokenizer)) != NULL) {
      switch (token_type(t)) {
      case TK_NEWLINE:
        fputs(token_value(t), b->stream);
        fflush(b->stream);
        push_line_end(b->line_ends, b->stream_len);
        break;
      case TK_CONTENT:
        fputs(token_value(t), b->stream);
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
    cleanup_buffer(b);
  }

  return n;
}

void buffer_read_all(Buffer *b) {
  while (buffer_read(b));
}

const char **get_buffer_lines(Buffer *b, size_t start, size_t num) {
  if (start >= b->line_ends->len) {
    num = 0;
  } else if (start + num > b->line_ends->len) {
    num = b->line_ends->len - start;
  }

  char **buffer_lines = malloc((num + 1) * sizeof *buffer_lines);
  size_t start_offset = 0;
  size_t line_len;

  for (size_t i = 0, *next_offset = b->line_ends->offsets; i < num; i++, next_offset++) {
    line_len = *next_offset - start_offset;

    buffer_lines[i] = (char*)malloc((line_len + 1) * sizeof(*buffer_lines[i]));
    memcpy((char*)buffer_lines[i], b->stream_str + start_offset, line_len);
    buffer_lines[i][line_len] = '\0';

    start_offset = *next_offset;
  }

  buffer_lines[num] = NULL;

  return (const char**)buffer_lines;
}

//void buffer_list_annotations(Buffer *b) {
//  Annotation *a;
//  fprintf(stderr, "Annotations for buffer:\n");
//
//  for (size_t i = 0; i < annotations_len(b->annotations); i++) {
//    a = b->annotations->items[i];
//
//    fprintf(stderr, "- [%s] %u %u %s\n", annotation_type(a), annotation_start(a), annotation_end(a), annotation_value(a));
//  }
//}

size_t buffer_len(Buffer *b) {
  return b->stream_len;
}

const char* buffer_content(Buffer *b, size_t offset) {
  if (offset > b->stream_len) {
    return NULL;
  }

  return b->stream_str + offset;
}

int buffer_is_running(Buffer *b) {
  return b->is_running;
}

void buffer_add_annotation(Buffer *b, Annotation *a) {
  annotations_add(b->annotations, a);
}
