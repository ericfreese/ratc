#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <vterm.h>

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
  VTerm *vt;
  VTermScreen *vts;
  size_t scroll;
  size_t cursor;
  Box *box;
  AnnotatorListItem *annotators;
  EventHandlers *eh;

  struct refs refs;
};

/* The content of the `rect` has changed, `data` is a pointer to a `Pager` */
static int term_damage(VTermRect rect, void *data) {
  //invalidate_terminal(data, rect.start_row, rect.end_row);
  return 1;
}

/* Maybe has to do with moving the "selection"? */
static int term_moverect(VTermRect dest, VTermRect src, void *data) {
  //invalidate_terminal(data, MIN(dest.start_row, src.start_row),
  //    MAX(dest.end_row, src.end_row));
  return 1;
}

/* The cursor location changed */
static int term_movecursor(VTermPos new, VTermPos old, int visible, void *data) {
  //Terminal *term = data;
  //term->cursor.row = new.row;
  //term->cursor.col = new.col;
  //invalidate_terminal(term, old.row, old.row + 1);
  //invalidate_terminal(term, new.row, new.row + 1);
  return 1;
}

/* Handler for various properties changing (e.g. cursor becoming invisible) */
static int term_settermprop(VTermProp prop, VTermValue *val, void *data) {
  //Terminal *term = data;

  //switch (prop) {
  //  case VTERM_PROP_ALTSCREEN:
  //    break;

  //  case VTERM_PROP_CURSORVISIBLE:
  //    term->cursor.visible = val->boolean;
  //    invalidate_terminal(term, term->cursor.row, term->cursor.row + 1);
  //    break;

  //  case VTERM_PROP_TITLE: {
  //    buf_T *buf = handle_get_buffer(term->buf_handle);
  //    buf_set_term_title(buf, val->string);
  //    break;
  //  }

  //  case VTERM_PROP_MOUSE:
  //    term->forward_mouse = (bool)val->number;
  //    break;

  //  default:
  //    return 0;
  //}

  return 1;
}

/* What do we do when the bell rings */
static int term_bell(void *data) {
  //ui_call_bell();
  return 1;
}

/* Scrollback push handler (from pangoterm). */
static int term_sb_push(int cols, const VTermScreenCell *cells, void *data) {
  //Terminal *term = data;

  //if (!term->sb_size) {
  //  return 0;
  //}

  //// copy vterm cells into sb_buffer
  //size_t c = (size_t)cols;
  //ScrollbackLine *sbrow = NULL;
  //if (term->sb_current == term->sb_size) {
  //  if (term->sb_buffer[term->sb_current - 1]->cols == c) {
  //    // Recycle old row if it's the right size
  //    sbrow = term->sb_buffer[term->sb_current - 1];
  //  } else {
  //    xfree(term->sb_buffer[term->sb_current - 1]);
  //  }

  //  // Make room at the start by shifting to the right.
  //  memmove(term->sb_buffer + 1, term->sb_buffer,
  //      sizeof(term->sb_buffer[0]) * (term->sb_current - 1));

  //} else if (term->sb_current > 0) {
  //  // Make room at the start by shifting to the right.
  //  memmove(term->sb_buffer + 1, term->sb_buffer,
  //      sizeof(term->sb_buffer[0]) * term->sb_current);
  //}

  //if (!sbrow) {
  //  sbrow = xmalloc(sizeof(ScrollbackLine) + c * sizeof(sbrow->cells[0]));
  //  sbrow->cols = c;
  //}

  //// New row is added at the start of the storage buffer.
  //term->sb_buffer[0] = sbrow;
  //if (term->sb_current < term->sb_size) {
  //  term->sb_current++;
  //}

  //if (term->sb_pending < (int)term->sb_size) {
  //  term->sb_pending++;
  //}

  //memcpy(sbrow->cells, cells, sizeof(cells[0]) * c);
  //pmap_put(ptr_t)(invalidated_terminals, term, NULL);

  return 1;
}

/* Scrollback pop handler (from pangoterm). */
static int term_sb_pop(int cols, VTermScreenCell *cells, void *data) {
  //Terminal *term = data;

  //if (!term->sb_current) {
  //  return 0;
  //}

  //if (term->sb_pending) {
  //  term->sb_pending--;
  //}

  //ScrollbackLine *sbrow = term->sb_buffer[0];
  //term->sb_current--;
  //// Forget the "popped" row by shifting the rest onto it.
  //memmove(term->sb_buffer, term->sb_buffer + 1,
  //    sizeof(term->sb_buffer[0]) * (term->sb_current));

  //size_t cols_to_copy = (size_t)cols;
  //if (cols_to_copy > sbrow->cols) {
  //  cols_to_copy = sbrow->cols;
  //}

  //// copy to vterm state
  //memcpy(cells, sbrow->cells, sizeof(cells[0]) * cols_to_copy);
  //for (size_t col = cols_to_copy; col < (size_t)cols; col++) {
  //  cells[col].chars[0] = 0;
  //  cells[col].width = 1;
  //}

  //xfree(sbrow);
  //pmap_put(ptr_t)(invalidated_terminals, term, NULL);

  return 1;
}

static VTermScreenCallbacks pager_vterm_screen_callbacks = {
  .damage      = term_damage,
  .moverect    = term_moverect,
  .movecursor  = term_movecursor,
  .settermprop = term_settermprop,
  .bell        = term_bell,
  //.resize      = term_resize,
  .sb_pushline = term_sb_push,
  .sb_popline  = term_sb_pop,
};

void free_pager(const struct refs *r) {
  Pager *p = container_of(r, Pager, refs);

  fprintf(stderr, "FREEING PAGER %p\n", p);

  AnnotatorListItem *next;
  for (AnnotatorListItem *ali = p->annotators; ali != NULL; ali = next) {
    next = ali->next;
    free_annotator_list_item(ali);
  }

  vterm_free(p->vt);

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

  p->vt = vterm_new(24, 40);
  vterm_set_utf8(p->vt, 1);

  // This came from neovim/terminal.c -- I don't think we need it
  // VTermState *state = vterm_obtain_state(p->vt);
  // for (int i = 0; i < 16; i++) {
  //   vterm_state_set_palette_color(state, i, ...)
  // }

  p->vts = vterm_obtain_screen(p->vt);
  vterm_screen_set_callbacks(p->vts, &pager_vterm_screen_callbacks, p);
  vterm_screen_reset(p->vts, 1);

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
  vterm_screen_reset(p->vts, 1);

  io_start_vterm(p->vt, p->cmd);

  // for (AnnotatorListItem *ali = p->annotators; ali != NULL; ali = ali->next) {
  //   io_start_annotating_buffer(p->buffer, ali->annotator);
  // }
}

void cancel_pager_command(Pager *p) {
  io_close_buffer(p->vt);
}

//Buffer *get_buffer(Pager *p) {
//  return p->buffer;
//}

#define TEXT_SIZE 40 * 24

void render_pager(Pager *p) {
  /* TODO: loop over rows/columns, call `vterm_screen_get_cell` for each, and draw */

  char text[TEXT_SIZE];

  VTermRect rect = {
    .start_col = 0,
    .end_col   = 40,
    .start_row = 0,
    .end_row   = 24,
  };

  int n = vterm_screen_get_text(p->vts, text, TEXT_SIZE - 1, rect);
  text[n] = '\0';

  fprintf(stderr, "pager text: '%s'\n", text);


  //if (!p->box) {
  //  return;
  //}

  //move(box_top(p->box) + p->cursor - p->scroll, box_left(p->box) + 1);
  //addstr("▶");

  //Box *line_box = new_box(box_left(p->box) + 3, box_top(p->box), box_width(p->box) - 3, box_height(p->box));
  //RenderLines *render_lines = get_render_lines(p->buffer, p->scroll, box_height(line_box));
  //render_lines_draw(render_lines, line_box);
  //free_render_lines(render_lines);
  //free_box(line_box);
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
  //p->annotators = new_annotator_list_item(ar, p->annotators);
  //io_start_annotating_buffer(p->buffer, ar);
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

Annotation *pager_get_cursor_annotation(Pager *p, const char *type) {
  //return buffer_annotation_at_line(p->buffer, type, p->cursor);
  return NULL;
}

void pager_move_cursor_to(Pager *p, ssize_t cursor) {
  //if (cursor < 0) {
  //  p->cursor = 0;
  //} else if (cursor > buffer_num_lines(p->buffer) - 1) {
  //  p->cursor = buffer_num_lines(p->buffer) - 1;
  //} else {
  //  p->cursor = cursor;
  //}

  //if (p->cursor < p->scroll) {
  //  pager_scroll_to(p, p->cursor);
  //} else if (p->cursor > p->scroll + box_height(p->box) - 1) {
  //  pager_scroll_to(p, p->cursor - (box_height(p->box) - 1));
  //}
}

void pager_move_cursor(Pager *p, ssize_t delta) {
  pager_move_cursor_to(p, p->cursor + delta);
}

void pager_scroll_to(Pager *p, ssize_t scroll) {
  //if (scroll < 0) {
  //  p->scroll = 0;
  //} else if (scroll > buffer_num_lines(p->buffer) - box_height(p->box)) {
  //  if (buffer_num_lines(p->buffer) > box_height(p->box)) {
  //    p->scroll = buffer_num_lines(p->buffer) - box_height(p->box);
  //  } else {
  //    p->scroll = 0;
  //  }
  //} else {
  //  p->scroll = scroll;
  //}

  //if (p->cursor < p->scroll) {
  //  pager_move_cursor_to(p, p->scroll);
  //} else if (p->cursor > p->scroll + box_height(p->box) - 1) {
  //  pager_move_cursor_to(p, p->scroll + box_height(p->box) - 1);
  //}
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
  //pager_move_cursor_to(p, buffer_num_lines(p->buffer));
}
