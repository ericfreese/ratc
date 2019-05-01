#pragma once

#include "annotator.h"
#include "buffer.h"
#include "widget.h"

typedef struct pager Pager;
Pager *new_pager(char* cmd);
void pager_ref_inc(Pager *p);
void pager_ref_dec(Pager *p);
void run_pager_command(Pager *p);
void cancel_pager_command(Pager *p);
Buffer *get_buffer(Pager *p);
void render_pager(Pager *p);
void set_pager_box(Pager *p, int left, int top, int width, int height);
void pager_add_annotator(Pager *p, Annotator *ar);
void pager_reload(Pager *p);
void pager_move_cursor(Pager *p, ssize_t delta);
void pager_move_cursor_to(Pager *p, ssize_t cursor);
void pager_scroll(Pager *p, ssize_t delta);
void pager_scroll_to(Pager *p, ssize_t scroll);
void pager_page_up(Pager *p);
void pager_page_down(Pager *p);
void pager_first_line(Pager *p);
void pager_last_line(Pager *p);
//Widget *new_pager_widget(Pager *p);
