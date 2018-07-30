#include <ncurses.h>

#include "event_handlers.h"
#include "key_seq.h"
#include "pager_stack.h"
#include "rat.h"

EventHandlers *handlers;
KeySeq *input_buffer;
PagerStack *pagers;

void rat_init() {
  handlers = new_event_handlers();
  input_buffer = new_key_seq();
  pagers = new_pager_stack();
}

void rat_cleanup() {
  // TODO
}

void rat_push_pager(Pager *p) {
  pager_stack_push(pagers, p);
}

void rat_pop_pager() {
  pager_stack_pop(pagers);
}

void rat_render() {
  render_pager_stack(pagers);
}

size_t rat_num_pagers() {
  return pager_stack_len(pagers);
}

void rat_add_event_listener(KeySeq *trigger, JSEventHandler *jeh) {
  event_handlers_add(handlers, trigger, jeh);
}

void rat_handle_input(int ch) {
  size_t n;

  key_seq_add(input_buffer, (char*)keyname(ch));

  if ((n = event_handlers_handle(handlers, input_buffer)) > 0) {
    free_key_seq(input_buffer);
    input_buffer = new_key_seq();
  }
}
