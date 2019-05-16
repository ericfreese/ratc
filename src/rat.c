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

void render() {
  erase();
  render_pager_stack(pagers);
  refresh();
}

void handle_input(int ch) {
  switch (ch) {
    case KEY_RESIZE:
      return;
    case 'Q':
      done = 1;
      break;
  }

  size_t n;

  key_seq_add(input_buffer, (char*)keyname(ch));

  Pager *focused_pager = pager_stack_top(pagers);

  if (focused_pager != NULL && (n = pager_handle_event(focused_pager, input_buffer)) > 0) {
    free_key_seq(input_buffer);
    input_buffer = new_key_seq();
  } else if ((n = event_handlers_handle(handlers, input_buffer)) > 0) {
    free_key_seq(input_buffer);
    input_buffer = new_key_seq();
  }
}

void handle_winch(int sig) {
  endwin();
  refresh();

  set_pager_stack_box(pagers, 0, 0, COLS, LINES);

  render();
}

void install_winch_handler() {
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_winch;

  if (sigaction(SIGWINCH, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

void rat_init() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  start_color();
  use_default_colors();

  install_winch_handler();

  handlers = new_event_handlers();
  input_buffer = new_key_seq();
  pagers = new_pager_stack(new_box(0, 0, COLS, LINES));

  duk_ctx = duk_create_heap(NULL, NULL, NULL, NULL, duk_fatal_handler);
  js_rat_setup(duk_ctx);

  io_init(handle_input);

  duk_eval_string_noresult(duk_ctx,
    "var logger = new Rat.Pager('git llll');"
    "logger.addAnnotator(new Rat.Annotator('./test-annotator pager | tee >(xxd >> debug.log)', 'bar'));"
    "Rat.push(logger);"

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

    "Rat.addEventListener(['a'], function() {"
      "Rat.getActivePager().addAnnotator(new Rat.Annotator('./test-annotator foo | tee >(xxd >> debug.log)', 'bar'));"
    "});"

    "Rat.addEventListener(['^R'], function() {"
      "Rat.getActivePager().reload();"
    "});"

    "Rat.addEventListener(['j'], function() {"
      "Rat.getActivePager().moveCursor(1);"
    "});"

    "Rat.addEventListener(['k'], function() {"
      "Rat.getActivePager().moveCursor(-1);"
    "});"

    "Rat.addEventListener(['^E'], function() {"
      "Rat.getActivePager().scroll(1);"
    "});"

    "Rat.addEventListener(['^Y'], function() {"
      "Rat.getActivePager().scroll(-1);"
    "});"

    "Rat.addEventListener(['^D'], function() {"
      "Rat.getActivePager().pageDown();"
    "});"

    "Rat.addEventListener(['^U'], function() {"
      "Rat.getActivePager().pageUp();"
    "});"

    "Rat.addEventListener(['g', 'g'], function() {"
      "Rat.getActivePager().firstLine();"
    "});"

    "Rat.addEventListener(['G'], function() {"
      "Rat.getActivePager().lastLine();"
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

Pager *rat_active_pager() {
  return pager_stack_top(pagers);
}

void rat_add_event_listener(KeySeq *trigger, JSEventHandler *jeh) {
  event_handlers_add(handlers, trigger, jeh);
}

void rat_run() {
  while (!done && pager_stack_len(pagers) > 0) {
    render();
    io_tick();
  }
}
