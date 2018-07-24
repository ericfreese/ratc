#include <stdlib.h>

#include "box.h"

struct box {
  int left;
  int top;
  int width;
  int height;
};

Box *new_box(int left, int top, int width, int height) {
  Box *b = malloc(sizeof *b);

  b->left = left;
  b->top = top;
  b->width = width;
  b->height = height;

  return b;
}

void free_box(Box *b) {
  free(b);
}

int box_left(Box *b) {
  return b->left;
}

void box_set_left(Box *b, int left) {
  b->left = left;
}

int box_top(Box *b) {
  return b->top;
}

void box_set_top(Box *b, int top) {
  b->top = top;
}

int box_width(Box *b) {
  return b->width;
}

void box_set_width(Box *b, int width) {
  b->width = width;
}

int box_height(Box *b) {
  return b->height;
}

void box_set_height(Box *b, int height) {
  b->height = height;
}
