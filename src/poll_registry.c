#include "poll_registry.h"
#include "annotate.h"

PollRegistry *poll_registry;

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
}

void poll_registry_remove(void *ptr) {
  PollItem *cursor = poll_registry->first;

  if (cursor->ptr == ptr) {
    poll_registry->first = cursor->next;
    poll_registry->len--;
    free(cursor);
    return;
  }

  while (cursor->next != NULL) {
    if (cursor->next->ptr == ptr) {
      if (cursor->next == poll_registry->last) {
        poll_registry->last = cursor;
        free(cursor->next);
        cursor->next = NULL;
      } else {
        free(cursor->next);
        cursor->next = cursor->next->next;
      }

      poll_registry->len--;
      return;
    }

    cursor = cursor->next;
  }
}

PollItems *poll_registry_poll_items() {
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

  for (PollItem *pi = poll_registry->first; pi != NULL; pi = pi->next) {
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

void free_poll_items(PollItems *pis) {
  free(pis->items);
  free(pis);
}
