#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "render_lines.h"
#include "box.h"

typedef struct render_span RenderSpan;
struct render_span {
  attr_t attr;
  const char *content;
  RenderSpan *next;
};

struct render_line {
  RenderSpan *first;
  RenderSpan *last;
  RenderLine *next;
};

RenderLine *new_render_line() {
  RenderLine *rl = malloc(sizeof *rl);
  rl->first = rl->last = NULL;
  rl->next = NULL;
  return rl;
}

void free_render_line(RenderLine *rl) {
  RenderSpan *next;

  for (RenderSpan *cursor = rl->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free((char*)cursor->content);
    free(cursor);
  }

  free(rl);
}

void render_line_add(RenderLine *rl, attr_t attr, const char *content) {
  RenderSpan *rs = malloc(sizeof *rs);
  rs->attr = attr;
  rs->content = strdup(content);
  rs->next = NULL;

  if (rl->first == NULL) {
    rl->first = rl->last = rs;
  } else {
    rl->last->next = rs;
    rl->last = rs;
  }
}

void render_line_draw(RenderLine *rl, int y, int x) {
  move(y, x);

  for (RenderSpan *rs = rl->first; rs != NULL; rs = rs->next) {
    attron(rs->attr);
    printw("%s", rs->content);
    attroff(rs->attr);
  }
}

struct render_lines {
  RenderLine *first;
  RenderLine *last;
};

RenderLines *new_render_lines() {
  RenderLines *rls = malloc(sizeof *rls);
  rls->first = rls->last = NULL;
  return rls;
}

void free_render_lines(RenderLines *rls) {
  RenderLine *next;

  for (RenderLine *cursor = rls->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free_render_line(cursor);
  }

  free(rls);
}

void render_lines_add(RenderLines *rls, RenderLine *rl) {
  if (rls->first == NULL) {
    rls->first = rls->last = rl;
  } else {
    rls->last->next = rl;
    rls->last = rl;
  }
}

void render_lines_draw(RenderLines *rls, Box *box) {
  size_t y = 0;

  for (RenderLine *rl = rls->first; rl != NULL; rl = rl->next, y++) {
    render_line_draw(rl, y + box_top(box), box_left(box));
  }
}
