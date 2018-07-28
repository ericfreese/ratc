#include "rat.h"
#include "pager_stack.h"

PagerStack *pager_stack;

void rat_init() {
  pager_stack = new_pager_stack();
}

void rat_cleanup() {
  pager_stack = new_pager_stack();
}

void rat_push_pager(Pager *p) {
  pager_stack_push(pager_stack, p);
}

void rat_pop_pager() {
  pager_stack_pop(pager_stack);
}

void rat_render() {
  render_pager_stack(pager_stack);
}

size_t rat_num_pagers() {
  return pager_stack_len(pager_stack);
}
