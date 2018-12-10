#include <stdio.h>

#include "buffer.h"
#include "highlights.h"

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
  char *stream_str;
  size_t stream_len;
  FILE *stream;

  LineEnds *line_ends;
  Annotations *annotations;
  Highlights *highlights;
};

Buffer *new_buffer() {
  Buffer *b = (Buffer*)malloc(sizeof(Buffer));

  b->stream_len = 0;
  b->stream = open_memstream(&b->stream_str, &b->stream_len);
  b->line_ends = new_line_ends();
  b->annotations = new_annotations();
  b->highlights = new_highlights();

  return b;
}

void free_buffer(Buffer *b) {
  free_highlights(b->highlights);
  free_annotations(b->annotations);
  fclose(b->stream);
  free(b->stream_str);
  free_line_ends(b->line_ends);
  free(b);
}

void buffer_handle_token(Buffer *b, Token *t) {
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
    highlights_start(b->highlights, token_termstyle(t), b->stream_len);
    break;
  }
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

void buffer_add_annotation(Buffer *b, Annotation *a) {
  annotations_add(b->annotations, a);
}
