#ifndef POLL_REGISTRY_H
#define POLL_REGISTRY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>

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

void poll_registry_init();
void poll_registry_cleanup();
void poll_registry_add(PollItemType prtype, void *ptr, int fd);
void poll_registry_remove(void *ptr);
PollItems *poll_registry_poll_items();
void free_poll_items(PollItems *pis);
struct pollfd *poll_registry_build_pfd(PollItems *pis);

#endif
