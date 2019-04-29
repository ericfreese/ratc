#pragma once

#include "pager.h"

typedef struct pager_stack PagerStack;

PagerStack *new_pager_stack();
void free_pager_stack(PagerStack *ps);
void pager_stack_push(PagerStack *ps, Pager *p);
void pager_stack_pop(PagerStack *ps);
Pager *pager_stack_top(PagerStack *ps);
void set_pager_stack_box(PagerStack *ps, int left, int top, int width, int height);
size_t pager_stack_len(PagerStack *ps);
void render_pager_stack(PagerStack *ps);
