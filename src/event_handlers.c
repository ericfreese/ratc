#include <stdlib.h>

#include "event_handlers.h"

typedef struct event_handlers_item EventHandlersItem;
struct event_handlers_item {
  KeySeq *trigger;
  JSEventHandler *jeh;
  EventHandlersItem *next;
};

EventHandlersItem *new_event_handlers_item(KeySeq *ks, JSEventHandler *jeh) {
  EventHandlersItem *ehi = malloc(sizeof *ehi);

  ehi->trigger = ks;
  ehi->jeh = jeh;
  ehi->next = NULL;

  return ehi;
}

void free_event_handlers_item(EventHandlersItem *ehi) {
  js_free_event_handler(ehi->jeh);
  free_key_seq(ehi->trigger);
  free(ehi);
}

struct event_handlers {
  EventHandlersItem *first;
  EventHandlersItem *last;
  size_t len;
};

EventHandlers *new_event_handlers() {
  EventHandlers *eh = malloc(sizeof *eh);

  eh->first = NULL;
  eh->last = NULL;

  return eh;
}

void free_event_handlers(EventHandlers *eh) {
  // TODO
}

void event_handlers_add(EventHandlers *ehs, KeySeq *ks, JSEventHandler *jeh) {
  EventHandlersItem *item = new_event_handlers_item(ks, jeh);

  // TODO: Prevent adding duplicates

  if (ehs->last == NULL) {
    ehs->first = item;
    ehs->last = item;
  } else {
    ehs->last->next = item;
    ehs->last = item;
  }

  ehs->len++;
}

size_t event_handlers_handle(EventHandlers *ehs, KeySeq *ks) {
  for (EventHandlersItem *cursor = ehs->first; cursor != NULL; cursor = cursor->next) {
    if (key_seq_ends_with(ks, cursor->trigger)) {
      js_run_event_handler(cursor->jeh);
      return 1;
    }
  }

  return 0;
}
