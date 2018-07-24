#ifndef PAGER_H
#define PAGER_H

#include "buffer.h"
#include "widget.h"

typedef struct pager Pager;
Pager *new_pager(char* cmd);
void free_pager(Pager *p);
void run_pager_command(Pager *p);
Buffer *get_buffer(Pager *p);
void render_pager(void *pptr);
void set_pager_box(void *pptr, int left, int top, int width, int height);
Box *get_pager_box(void *pptr);
Widget *new_pager_widget(Pager *p);

#endif
