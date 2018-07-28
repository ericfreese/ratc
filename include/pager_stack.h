#ifndef PAGER_STACK_H
#define PAGER_STACK_H

#include "pager.h"

typedef struct pager_stack PagerStack;

PagerStack *new_pager_stack();
void free_pager_stack(PagerStack *ps);
void pager_stack_push(PagerStack *ps, Pager *p);
void pager_stack_pop(PagerStack *ps);
size_t pager_stack_len(PagerStack *ps);
void render_pager_stack(PagerStack *ps);

#endif