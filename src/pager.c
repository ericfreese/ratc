#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>

#include "pager.h"
#include "refs.h"
#include "io.h"

typedef struct annotator_list_item AnnotatorListItem;
struct annotator_list_item {
  AnnotatorListItem *next;
  Annotator *annotator;
};

AnnotatorListItem *new_annotator_list_item(Annotator *ar, AnnotatorListItem *next) {
  AnnotatorListItem *ali = malloc(sizeof *ali);

  annotator_ref_inc(ar);
  ali->annotator = ar;
  ali->next = next;

  return ali;
}

void free_annotator_list_item(AnnotatorListItem *ali) {
  annotator_ref_dec(ali->annotator);
  free(ali);
}

struct pager {
  char *cmd;
  Buffer *buffer;
  int scroll;
  int cursor;
  Box *box;
  AnnotatorListItem *annotators;

  struct refs refs;
};

void free_pager(const struct refs *r) {
  Pager *p = container_of(r, Pager, refs);

  fprintf(stderr, "FREEING PAGER %p\n", p);

  AnnotatorListItem *next;
  for (AnnotatorListItem *cursor = p->annotators; cursor != NULL; cursor = next) {
    next = cursor->next;
    free_annotator_list_item(cursor);
  }

  // TODO: Can we remove this check? We should always have a buffer right?
  if (p->buffer) {
    free_buffer(p->buffer);
  }

  free_box(p->box);

  free(p);
}

Pager *new_pager(char *cmd) {
  Pager *p = (Pager*)malloc(sizeof(*p));

  p->cmd = cmd;
  p->box = new_box(3, 3, 100, 20);
  p->refs = (struct refs){free_pager, 1};
  p->annotators = NULL;

  run_pager_command(p);

  return p;
}

void pager_ref_inc(Pager *p) {
  ref_inc(&p->refs);
}

void pager_ref_dec(Pager *p) {
  ref_dec(&p->refs);
}

void run_pager_command(Pager *p) {
  p->buffer = new_buffer();

  io_start_buffer(p->buffer, p->cmd);

  for (AnnotatorListItem *cursor = p->annotators; cursor != NULL; cursor = cursor->next) {
    io_start_annotating_buffer(p->buffer, cursor->annotator);
  }
}

void cancel_pager_command(Pager *p) {
  io_close_buffer(p->buffer);
}

Buffer *get_buffer(Pager *p) {
  return p->buffer;
}

void render_pager(Pager *p) {
  const char **buffer_lines;

  buffer_lines = get_buffer_lines(p->buffer, 0, box_height(p->box));

  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  attron(COLOR_PAIR(1));

  for (int y = 0; buffer_lines[y] != NULL; y++) {
    mvprintw(y + box_top(p->box), box_left(p->box), "%s", buffer_lines[y]);
    free((char*)buffer_lines[y]);
  }

  attroff(COLOR_PAIR(2));

  free(buffer_lines);
}

void set_pager_box(Pager *p, int left, int top, int width, int height) {
  box_set_left(p->box, left);
  box_set_top(p->box, top);
  box_set_width(p->box, width);
  box_set_height(p->box, height);
}

Box *get_pager_box(Pager *p) {
  return p->box;
}

void pager_add_annotator(Pager *p, Annotator *ar) {
  p->annotators = new_annotator_list_item(ar, p->annotators);
  io_start_annotating_buffer(p->buffer, ar);
}

//Widget *new_pager_widget(Pager *p) {
//  return new_widget(p, render_pager, set_pager_box, get_pager_box);
//}
