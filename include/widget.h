#pragma once

#include "box.h"

typedef void (*wfn_render)(void *);
typedef void (*wfn_set_box)(void *, int, int, int, int);
typedef Box *(*wfn_get_box)(void *);

typedef struct widget Widget;
Widget *new_widget(void *ptr, wfn_render render, wfn_set_box set_box, wfn_get_box get_box);
void render_widget(Widget *w);
void set_widget_box(Widget *w, int left, int top, int width, int height);
Box *get_widget_box(Widget *w);
