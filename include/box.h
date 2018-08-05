#pragma once

typedef struct box Box;
Box *new_box(int left, int top, int width, int height);
void free_box(Box *b);
int box_left(Box *b);
void box_set_left(Box *b, int left);
int box_top(Box *b);
void box_set_top(Box *b, int top);
int box_width(Box *b);
void box_set_width(Box *b, int width);
int box_height(Box *b);
void box_set_height(Box *b, int height);
