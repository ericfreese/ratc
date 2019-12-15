#pragma once

#include "event_handlers.h"
#include "key_seq.h"
#include "pager.h"

void rat_init();
void rat_run();
void rat_cleanup();
void rat_push_pager(Pager *p);
void rat_pop_pager();
Pager *rat_active_pager();
Pager *rat_get_pager(ssize_t offset);
void rat_add_event_listener(KeySeq *trigger, JSEventHandler *jeh);
