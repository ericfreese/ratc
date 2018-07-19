#include "poll_registry.h"
#include "annotate.h"

PollRegistry *poll_registry;

PollItems *new_poll_items() {
  PollItems *pis;

  if ((pis = malloc(sizeof(*pis))) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  pis->len = 0;

  if ((pis->items = malloc(poll_registry->len * sizeof(*pis))) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  return pis;
}

void poll_items_add(PollItems *pis, PollItem *pi) {
  if (pis->len >= poll_registry->len) {
    return;
  }

  pis->items[pis->len] = pi;
  pis->len++;
}

void free_poll_items(PollItems *pis) {
  free(pis->items);
  free(pis);
}

void poll_registry_init() {
  if ((poll_registry = malloc(sizeof(*poll_registry))) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  poll_registry->first = NULL;
  poll_registry->last = NULL;
}

void poll_registry_cleanup() {
  PollItem *next;

  for (PollItem *cursor = poll_registry->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free(cursor);
  }

  free(poll_registry);
}

void poll_registry_print(char *str) {
  fprintf(stderr, "Poll registry (%s, %ld, first: %p, last: %p):\n", str, poll_registry->len, poll_registry->first, poll_registry->last);

  for (PollItem *pi = poll_registry->first; pi != NULL; pi = pi->next) {
    fprintf(stderr, "  - %p -- type: %d, fd: %d, ptr: %p, next: %p\n", pi, pi->type, pi->fd, pi->ptr, pi->next);
  }
}

void poll_registry_add(PollItemType type, void *ptr, int fd) {
  PollItem *pi;

  if ((pi = malloc(sizeof(*pi))) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  pi->next = NULL;
  pi->type = type;
  pi->ptr = ptr;
  pi->fd = fd;

  fprintf(stderr, "registering poll item %p, %d\n", ptr, fd);

  if (poll_registry->last == NULL) {
    poll_registry->first = pi;
    poll_registry->last = pi;
  } else {
    poll_registry->last->next = pi;
    poll_registry->last = pi;
  }

  poll_registry->len++;
  poll_registry_print("added");
}

void poll_registry_remove(int fd) {
  PollItem *cursor = poll_registry->first;
  PollItem *to_remove = NULL;

  fprintf(stderr, "removing poll item %d\n", fd);

  if (cursor == NULL) {
    return;
  }

  if (cursor->fd == fd) {
    to_remove = cursor;
  } else {
    while (cursor->next != NULL) {
      if (cursor->next->fd == fd) {
        to_remove = cursor->next;
        break;
      }

      cursor = cursor->next;
    }
  }

  if (to_remove == NULL) {
    fprintf(stderr, "could not find poll item %d\n", fd);
    return;
  }

  if (to_remove == poll_registry->first) {
    if (cursor->next != NULL) {
      poll_registry->first = cursor->next;
    } else {
      poll_registry->first = poll_registry->last = NULL;
    }
  } else if (to_remove == poll_registry->last) {
    poll_registry->last = cursor;
    cursor->next = NULL;
  } else {
    cursor->next = cursor->next->next;
  }

  free(to_remove);
  poll_registry->len--;
  poll_registry_print("removed");
}

PollItems *poll_registry_poll_items() {
  PollItems *pis = new_poll_items();

  for (PollItem *pi = poll_registry->first; pi != NULL; pi = pi->next) {
    Annotator *a;

    switch (pi->type) {
      case PI_USER_INPUT:
      case PI_BUFFER_READ:
      case PI_ANNOTATOR_READ:
        poll_items_add(pis, pi);
        break;
      case PI_ANNOTATOR_WRITE:
        a = (Annotator*)pi->ptr;

        fprintf(stderr, "building poll, annotator at: %ld/%ld %d\n", a->woffset, a->buffer->stream_len, a->buffer->is_running);

        if (a->woffset < a->buffer->stream_len || !a->buffer->is_running) {
          poll_items_add(pis, pi);
        }

        break;
    }
  }

  return pis;
}

struct pollfd *poll_registry_build_pfd(PollItems *pis) {
  struct pollfd *pfd;

  if ((pfd = malloc(pis->len * sizeof(*pfd))) == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

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
