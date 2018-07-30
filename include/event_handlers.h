#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <stdlib.h>

#include "js_api.h"
#include "key_seq.h"

typedef struct event_handlers EventHandlers;
EventHandlers *new_event_handlers();
void free_event_handlers(EventHandlers *ehs);
void event_handlers_add(EventHandlers *ehs, KeySeq *trigger, JSEventHandler *jeh);
size_t event_handlers_handle(EventHandlers *ehs, KeySeq *ks);

#endif
