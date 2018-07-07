#include "poll_registry.h"

PollRegistry *new_poll_registry() {
  PollRegistry *pr = malloc(sizeof(*pr));

  pr->first = NULL;
  pr->last = NULL;

  return pr;
}

void free_poll_registry(PollRegistry *pr) {
  // TODO: Will we ever need this? The registry will probably be a global singleton
}

void poll_registry_add(PollRegistry *pr, PollItemType type, void *ptr, int fd) {
  PollItem *pi = malloc(sizeof(*pi));

  pi->next = NULL;
  pi->type = type;
  pi->ptr = ptr;
  pi->fd = fd;

  fprintf(stderr, "registering poll item %p, %d\n", ptr, fd);

  if (pr->last == NULL) {
    pr->first = pi;
    pr->last = pi;
  } else {
    pr->last->next = pi;
    pr->last = pi;
  }

  pr->len++;
}

void poll_registry_remove(PollRegistry *pr, void *ptr) {
  PollItem *cursor = pr->first;

  if (cursor->ptr == ptr) {
    pr->first = cursor->next;
    pr->len--;
    free(cursor);
    return;
  }

  while (cursor->next != NULL) {
    if (cursor->next->ptr == ptr) {
      if (cursor->next == pr->last) {
        pr->last = cursor;
        free(cursor->next);
        cursor->next = NULL;
      } else {
        free(cursor->next);
        cursor->next = cursor->next->next;
      }

      pr->len--;
      return;
    }

    cursor = cursor->next;
  }
}

PollItems *poll_registry_poll_items(PollRegistry *pr) {
  PollItems *pis = malloc(sizeof(*pis));
  pis->len = 0;
  pis->items = malloc(pr->len * sizeof(*pis));

  for (PollItem *pi = pr->first; pi != NULL; pi = pi->next) {
    switch (pi->type) {
      case PI_USER_INPUT:
      case PI_BUFFER_READ:
        pis->items[pis->len] = pi;
        pis->len++;
        break;
      case PI_ANNOTATOR_READ:
        break;
      case PI_ANNOTATOR_WRITE:
        fprintf(stderr, "building poll, annotator at: %ld/%ld\n", ((Annotator*)pi->ptr)->woffset, ((Annotator*)pi->ptr)->buffer->stream_len);
        if (((Annotator*)pi->ptr)->woffset < ((Annotator*)pi->ptr)->buffer->stream_len) {
          pis->items[pis->len] = pi;
          pis->len++;
        }

        break;
    }
  }

  return pis;
}

struct pollfd *poll_registry_build_pfd(PollItems *pis) {
  struct pollfd *pfd = malloc(pis->len * sizeof(*pfd));

  for (int i = 0; i < pis->len; i++) {
    pfd[i].fd = pis->items[i]->fd;

    switch (pis->items[i]->type) {
      case PI_USER_INPUT:
      case PI_BUFFER_READ:
      case PI_ANNOTATOR_READ:
        pfd[i].events = POLLIN;
        break;
      case PI_ANNOTATOR_WRITE:
        pfd[i].events = POLLOUT;
        break;
    }
  }

  return pfd;
}

void free_poll_items(PollItems *pis) {
  free(pis->items);
  free(pis);
}
