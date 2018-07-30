#ifndef RAT_H
#define RAT_H

#include "event_handlers.h"
#include "key_seq.h"
#include "pager.h"

void rat_init();
void rat_cleanup();
void rat_push_pager(Pager *p);
void rat_pop_pager();
void rat_render();
size_t rat_num_pagers();
void rat_add_event_listener(KeySeq *trigger, JSEventHandler *jeh);
void rat_handle_input(int ch);

#endif
