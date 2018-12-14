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
};

void pager_stack_print(PagerStack *ps, char *str) {
  fprintf(stderr, "Pager Stack (%s, %ld, first: %p, last: %p):\n", str, ps->len, ps->first, ps->last);

  for (PagerStackItem *psi = ps->first; psi != NULL; psi = psi->next) {
    fprintf(stderr, "  - %p -- pager: %p, previous: %p, next: %p\n", psi, psi->pager, psi->previous, psi->next);
  }
}

PagerStack *new_pager_stack() {
  PagerStack *ps = malloc(sizeof *ps);

  ps->first = ps->last = NULL;
  ps->len = 0;

  return ps;
}

void free_pager_stack(PagerStack *ps) {
  // TODO: Used as global singleton, so not really necessary for now
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

  pager_stack_print(ps, "after pop");
}

Pager *pager_stack_top(PagerStack *ps) {
  if (ps->last) {
    return ps->last->pager;
  }

  return NULL;
}

size_t pager_stack_len(PagerStack *ps) {
  return ps->len;
}

void render_pager_stack(PagerStack *ps) {
  if (ps->last == NULL) {
    return;
  }

  render_pager(ps->last->pager);
}
