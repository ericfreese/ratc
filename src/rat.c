#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <ncurses.h>

#include "event_handlers.h"
#include "key_seq.h"
#include "pager_stack.h"
#include "rat.h"
#include "io.h"

EventHandlers *handlers;
KeySeq *input_buffer;
PagerStack *pagers;
duk_context *duk_ctx;

int done = 0;

static void duk_fatal_handler(void *udata, const char *msg) {
  fprintf(stderr, "FATAL: %s\n", msg);
  abort();
}

void handle_input(int ch) {
  switch (ch) {
    case 'Q':
      done = 1;
      break;
  }

  size_t n;

  key_seq_add(input_buffer, (char*)keyname(ch));

  if ((n = event_handlers_handle(handlers, input_buffer)) > 0) {
    free_key_seq(input_buffer);
    input_buffer = new_key_seq();
  }
}

void rat_init() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  start_color();

  handlers = new_event_handlers();
  input_buffer = new_key_seq();
  pagers = new_pager_stack();

  duk_ctx = duk_create_heap(NULL, NULL, NULL, NULL, duk_fatal_handler);
  js_rat_setup(duk_ctx);

  io_init(handle_input);

  duk_eval_string_noresult(duk_ctx,
    "Rat.addEventListener(['p'], function() {"
      "var p = new Rat.Pager('for i ({1..15}); do echo foo; sleep 1; done');"
      "p.addAnnotator(new Rat.Annotator('./test-annotator foo | tee >(xxd >> debug.log)', 'bar'));"
      "Rat.push(p);"
    "});"

    "Rat.addEventListener(['d'], function() {"
      "var p = new Rat.Pager('git diff --no-color');"
      "p.addAnnotator(new Rat.Annotator('./test-annotator pager | tee >(xxd >> debug.log)', 'bar'));"
      "Rat.push(p);"
    "});"

    "Rat.addEventListener(['l'], function() {"
      "var p = new Rat.Pager('git llll --no-color');"
      "p.addAnnotator(new Rat.Annotator('./test-annotator pager | tee >(xxd >> debug.log)', 'bar'));"
      "Rat.push(p);"
    "});"

    "Rat.addEventListener(['q'], function() {"
      "Rat.pop();"
    "});"
  );
}

void rat_cleanup() {
  // TODO: clean up everything else

  io_cleanup();

  duk_destroy_heap(duk_ctx);
  endwin();
}

void rat_push_pager(Pager *p) {
  pager_stack_push(pagers, p);
}

void rat_pop_pager() {
  pager_stack_pop(pagers);
}

void rat_add_event_listener(KeySeq *trigger, JSEventHandler *jeh) {
  event_handlers_add(handlers, trigger, jeh);
}

void render() {
  erase();
  render_pager_stack(pagers);
  refresh();
}

void rat_run() {
  while (!done) {
    render();
    io_tick();
  }
}
