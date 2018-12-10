#include <stdio.h>

#include "buffer.h"
#include "render_lines.h"

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

typedef struct highlight Highlight;
struct highlight {
  TermStyle *ts;
  size_t start;
  size_t end;
  Highlight *next;
};

typedef struct highlights Highlights;
struct highlights {
  Highlight *first;
  Highlight *last;
  int highlighting;
};

Highlights *new_highlights() {
  Highlights *hs = malloc(sizeof *hs);

  hs->first = hs->last = NULL;
  hs->highlighting = 0;

  return hs;
}

void free_highlights(Highlights *hs) {
  Highlight *next;

  for (Highlight *cursor = hs->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free(cursor->ts);
    free(cursor);
  }

  free(hs);
}

void highlights_end(Highlights *hs, size_t offset) {
  if (hs->last != NULL) {
    hs->last->end = offset;
  }

  hs->highlighting = 0;
}

void highlights_start(Highlights *hs, TermStyle *ts, size_t offset) {
  if (hs->highlighting) {
    highlights_end(hs, offset);
  }

  if (term_style_is_default(ts)) {
    return;
  }

  fprintf(stderr, "highlighting termstyle: fg='%d', bg='%d'\n", ts->fg, ts->bg);

  Highlight *h = malloc(sizeof *h);

  h->ts = term_style_dup(ts);
  h->start = offset;
  h->end = SIZE_MAX;
  h->next = NULL;

  if (hs->first == NULL) {
    hs->first = hs->last = h;
  } else {
    hs->last->next = h;
    hs->last = h;
  }

  hs->highlighting = 1;
}

// TODO: Implement as some more efficient data structure
// - Tango tree?
// - Red black tree? http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
// - Splay tree?
// - Skip list w/ finger search? http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_skip.aspx
Highlight *highlight_at_point(Highlights *hs, size_t point) {
  for (Highlight *cursor = hs->first; cursor != NULL && cursor->start <= point; cursor = cursor->next) {
    if (point >= cursor->start && point < cursor->end) {
      return cursor;
    }
  }

  return NULL;
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

RenderLines *get_render_lines(Buffer *b, size_t start, size_t num) {
  if (start >= b->line_ends->len) {
    num = 0;
  } else if (start + num > b->line_ends->len) {
    num = b->line_ends->len - start;
  }

  RenderLines *rls = new_render_lines();

  size_t start_offset;
  if (start > 1) {
    start_offset = b->line_ends->offsets[start - 1];
  } else {
    start_offset = 0;
  }

  size_t line_len;

  size_t n = 0;
  size_t *next_offset = &b->line_ends->offsets[start];
  //Highlight *highlight = highlight_at_point(b->highlights, start_offset);

  char *content;

  while (n < num) {
    RenderLine *rl = new_render_line();

    line_len = *next_offset - start_offset;

    content = malloc(line_len + 1);
    memcpy(content, b->stream_str + start_offset, line_len);
    content[line_len] = '\0';

    render_line_add(rl, A_BOLD, content);
    free(content);

    start_offset = *next_offset;

    render_lines_add(rls, rl);

    n++;
    next_offset++;
  }

  return rls;
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
