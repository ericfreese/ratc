#ifndef POLL_INFO_H
#define POLL_INFO_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include "annotate.h"

typedef enum {
  PI_USER_INPUT = 0,
  PI_BUFFER_READ,
  PI_ANNOTATOR_WRITE,
  PI_ANNOTATOR_READ
} PollItemType;

typedef struct PollItem PollItem;
struct PollItem {
  PollItem *next;
  PollItemType type;
  void *ptr;
  int fd;
};

typedef struct {
  PollItem *first;
  PollItem *last;
  size_t len;
} PollRegistry;

typedef struct {
  size_t len;
  size_t size;
  PollItem **items;
} PollItems;

PollRegistry *new_poll_registry();
void free_poll_registry(PollRegistry *pr);
void poll_registry_add(PollRegistry *pr, PollItemType prtype, void *ptr, int fd);
void poll_registry_remove(PollRegistry *pr, void *ptr);
PollItems *poll_registry_poll_items(PollRegistry *pr);
struct pollfd *poll_registry_build_pfd(PollItems *pis);
void free_poll_items(PollItems *pis);

#endif
