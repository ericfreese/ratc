#include <stdlib.h>

#include "widget.h"

struct widget {
  void *ptr;
  wfn_render render;
  wfn_set_box set_box;
  wfn_get_box get_box;
};

Widget *new_widget(void *ptr, wfn_render render, wfn_set_box set_box, wfn_get_box get_box) {
  Widget *w = malloc(sizeof *w);

  w->ptr = ptr;
  w->render = render;
  w->set_box = set_box;
  w->get_box = get_box;

  return w;
}

void render_widget(Widget *w) {
  w->render(w->ptr);
}

void set_widget_box(Widget *w, int left, int top, int width, int height) {
  w->set_box(w->ptr, left, top, width, height);
}

Box *get_widget_box(Widget *w) {
  return w->get_box(w->ptr);
}
