#include <ncurses.h>

#include "highlights.h"

struct highlight {
  TermStyle *ts;
  size_t start;
  size_t end;
  Highlight *next;
};

TermStyle *highlight_get_style(Highlight *h) {
  return h->ts;
}

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
  h->end = offset;

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
    if (point >= cursor->start && (point < cursor->end || (cursor == hs->last && hs->highlighting))) {
      return cursor;
    }
  }

  return NULL;
}
