#include "pager_stack.h"

typedef struct pager_stack_item PagerStackItem;
struct pager_stack_item {
  PagerStackItem *next;
  PagerStackItem *previous;
  Pager *pager;
};

PagerStackItem *new_pager_stack_item(Pager *p) {
  PagerStackItem *psi = malloc(sizeof *psi);

  psi->next = NULL;
  psi->previous = NULL;
  psi->pager = p;

  pager_ref_inc(psi->pager);

  return psi;
}

void free_pager_stack_item(PagerStackItem *psi) {
  cancel_pager_command(psi->pager);
  pager_ref_dec(psi->pager);
  free(psi);
}

struct pager_stack {
  PagerStackItem *first;
  PagerStackItem *last;
  size_t len;
  int max_visible;
  int has_valid_layout;
  Box *box;
};

void pager_stack_print(PagerStack *ps, char *str) {
  fprintf(stderr, "Pager Stack (%s, %ld, first: %p, last: %p):\n", str, ps->len, ps->first, ps->last);

  for (PagerStackItem *psi = ps->first; psi != NULL; psi = psi->next) {
    fprintf(stderr, "  - %p -- pager: %p, previous: %p, next: %p\n", psi, psi->pager, psi->previous, psi->next);
  }
}

PagerStack *new_pager_stack(Box *box) {
  PagerStack *ps = malloc(sizeof *ps);

  ps->first = ps->last = NULL;
  ps->len = 0;
  ps->max_visible = 3;
  ps->has_valid_layout = 0;
  ps->box = box;

  return ps;
}

void free_pager_stack(PagerStack *ps) {
  // TODO: Used as global singleton, so not really necessary for now
  // free_box(ps->box);
}

void pager_stack_push(PagerStack *ps, Pager *p) {
  PagerStackItem *psi = new_pager_stack_item(p);

  if (ps->last == NULL) {
    ps->first = ps->last = psi;
  } else {
    psi->previous = ps->last;
    ps->last->next = psi;
    ps->last = psi;
  }

  ps->len++;
  ps->has_valid_layout = 0;

  pager_stack_print(ps, "after push");
}

void pager_stack_pop(PagerStack *ps) {
  PagerStackItem *to_remove = ps->last;

  if (to_remove == NULL) {
    return;
  }

  if (to_remove->previous == NULL) {
    ps->first = ps->last = NULL;
  } else {
    to_remove->previous->next = NULL;
    ps->last = to_remove->previous;
  }

  free_pager_stack_item(to_remove);
  ps->len--;
  ps->has_valid_layout = 0;

  pager_stack_print(ps, "after pop");
}

Pager *pager_stack_get(PagerStack *ps, ssize_t offset) {
  PagerStackItem *psi;

  for (psi = ps->last; psi && offset; psi = psi->previous, offset--);

  if (!offset && psi) {
    return psi->pager;
  }

  return NULL;
}

size_t pager_stack_len(PagerStack *ps) {
  return ps->len;
}

PagerStackItem *pager_stack_first_visible_item(PagerStack *ps) {
  if (!ps->last) {
    return NULL;
  }

  PagerStackItem *psi;
  PagerStackItem *first_visible;
  int i;

  for (psi = ps->last, i = 0; psi != NULL && i < ps->max_visible; psi = psi->previous, i++) {
    first_visible = psi;
  }

  return first_visible;
}

void set_pager_stack_box(PagerStack *ps, int left, int top, int width, int height) {
  box_set_left(ps->box, left);
  box_set_top(ps->box, top);
  box_set_width(ps->box, width);
  box_set_height(ps->box, height);

  ps->has_valid_layout = 0;
}

void pager_stack_layout(PagerStack *ps) {
  if (ps->last == NULL) {
    return;
  }

  PagerStackItem *psi;
  int i, size;
  int n = ps->len > ps->max_visible ? ps->max_visible : ps->len;
  int total_size = box_width(ps->box);
  int remaining = total_size;
  int offset = 0;

  for (psi = pager_stack_first_visible_item(ps), i = 0; psi != NULL && i < n; psi = psi->next, i++) {
    size = (remaining - (n - i - 1)) / (n - i);
    set_pager_box(psi->pager, offset, 0, size, box_height(ps->box));
    offset = offset + size + 1;
    remaining = total_size - offset;
  }

  ps->has_valid_layout = true;
}

void render_pager_stack(PagerStack *ps) {
  if (ps->last == NULL) {
    return;
  }

  if (!ps->has_valid_layout) {
    pager_stack_layout(ps);
  }

  PagerStackItem *psi;

  for (psi = pager_stack_first_visible_item(ps); psi != NULL; psi = psi->next) {
    render_pager(psi->pager);
  }
}
