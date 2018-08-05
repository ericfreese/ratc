#ifndef PAGER_H
#define PAGER_H

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
Box *get_pager_box(Pager *p);
void pager_add_annotator(Pager *p, Annotator *ar);
//Widget *new_pager_widget(Pager *p);

#endif
