#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>

#include "pager.h"
#include "refs.h"
#include "render_lines.h"
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
  size_t scroll;
  size_t cursor;
  Box *box;
  AnnotatorListItem *annotators;
  EventHandlers *eh;

  struct refs refs;
};

void free_pager(const struct refs *r) {
  Pager *p = container_of(r, Pager, refs);

  fprintf(stderr, "FREEING PAGER %p\n", p);

  AnnotatorListItem *next;
  for (AnnotatorListItem *ali = p->annotators; ali != NULL; ali = next) {
    next = ali->next;
    free_annotator_list_item(ali);
  }

  // TODO: Can we remove this check? We should always have a buffer right?
  if (p->buffer) {
    free_buffer(p->buffer);
  }

  if(p->box) {
    free_box(p->box);
  }

  free_event_handlers(p->eh);

  free(p->cmd);
  free(p);
}

Pager *new_pager(char *cmd) {
  Pager *p = (Pager*)malloc(sizeof(*p));

  p->cmd = cmd;
  p->box = NULL;
  p->refs = (struct refs){free_pager, 1};
  p->annotators = NULL;
  p->buffer = NULL;
  p->scroll = p->cursor = 0;
  p->eh = new_event_handlers();

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
  if(p->buffer) {
    free_buffer(p->buffer);
  }

  p->buffer = new_buffer();

  io_start_buffer(p->buffer, p->cmd);

  for (AnnotatorListItem *ali = p->annotators; ali != NULL; ali = ali->next) {
    io_start_annotating_buffer(p->buffer, ali->annotator);
  }
}

void cancel_pager_command(Pager *p) {
  io_close_buffer(p->buffer);
}

Buffer *get_buffer(Pager *p) {
  return p->buffer;
}

void render_pager(Pager *p) {
  if (!p->box) {
    return;
  }

  move(box_top(p->box) + p->cursor - p->scroll, box_left(p->box) + 1);
  addstr("▶");

  Box *line_box = new_box(box_left(p->box) + 3, box_top(p->box), box_width(p->box) - 3, box_height(p->box));
  RenderLines *render_lines = get_render_lines(p->buffer, p->scroll, box_height(line_box));
  render_lines_draw(render_lines, line_box);
  free_render_lines(render_lines);
  free_box(line_box);
}

void set_pager_box(Pager *p, int left, int top, int width, int height) {
  if (p->box) {
    box_set_left(p->box, left);
    box_set_top(p->box, top);
    box_set_width(p->box, width);
    box_set_height(p->box, height);
  } else {
    p->box = new_box(left, top, width, height);
  }

  pager_scroll_to(p, p->scroll);
}

void pager_add_annotator(Pager *p, Annotator *ar) {
  p->annotators = new_annotator_list_item(ar, p->annotators);
  io_start_annotating_buffer(p->buffer, ar);
}

void pager_reload(Pager *p) {
  cancel_pager_command(p);
  run_pager_command(p);
}

void pager_add_event_listener(Pager *p, KeySeq *ks, JSEventHandler *jeh) {
  event_handlers_add(p->eh, ks, jeh);
}

size_t pager_handle_event(Pager *p, KeySeq *ks) {
  return event_handlers_handle(p->eh, ks);
}

void pager_move_cursor_to(Pager *p, ssize_t cursor) {
  if (cursor < 0) {
    p->cursor = 0;
  } else if (cursor > buffer_num_lines(p->buffer) - 1) {
    p->cursor = buffer_num_lines(p->buffer) - 1;
  } else {
    p->cursor = cursor;
  }

  if (p->cursor < p->scroll) {
    pager_scroll_to(p, p->cursor);
  } else if (p->cursor > p->scroll + box_height(p->box) - 1) {
    pager_scroll_to(p, p->cursor - (box_height(p->box) - 1));
  }
}

void pager_move_cursor(Pager *p, ssize_t delta) {
  pager_move_cursor_to(p, p->cursor + delta);
}

void pager_scroll_to(Pager *p, ssize_t scroll) {
  if (scroll < 0) {
    p->scroll = 0;
  } else if (scroll > buffer_num_lines(p->buffer) - box_height(p->box)) {
    if (buffer_num_lines(p->buffer) > box_height(p->box)) {
      p->scroll = buffer_num_lines(p->buffer) - box_height(p->box);
    } else {
      p->scroll = 0;
    }
  } else {
    p->scroll = scroll;
  }

  if (p->cursor < p->scroll) {
    pager_move_cursor_to(p, p->scroll);
  } else if (p->cursor > p->scroll + box_height(p->box) - 1) {
    pager_move_cursor_to(p, p->scroll + box_height(p->box) - 1);
  }
}

void pager_scroll(Pager *p, ssize_t delta) {
  pager_scroll_to(p, p->scroll + delta);
}

void pager_page_up(Pager *p) {
  pager_scroll(p, -box_height(p->box));
}

void pager_page_down(Pager *p) {
  pager_scroll(p, box_height(p->box));
}

void pager_first_line(Pager *p) {
  pager_move_cursor_to(p, 0);
}

void pager_last_line(Pager *p) {
  pager_move_cursor_to(p, buffer_num_lines(p->buffer));
}
