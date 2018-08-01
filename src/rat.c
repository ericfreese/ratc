#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <ncurses.h>

#include "event_handlers.h"
#include "key_seq.h"
#include "pager_stack.h"
#include "poll_registry.h"
#include "rat.h"

EventHandlers *handlers;
KeySeq *input_buffer;
PagerStack *pagers;
duk_context *duk_ctx;

int done = 0;

static void duk_fatal_handler(void *udata, const char *msg) {
  fprintf(stderr, "FATAL: %s\n", msg);
  abort();
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

  poll_registry_init();
  poll_registry_add(PI_USER_INPUT, NULL, open("/dev/tty", O_RDONLY));

  duk_eval_string_noresult(duk_ctx,
    "Rat.addEventListener(['p'], function() {"
      "Rat.print('IT WORKED');"
      "Rat.push(new Rat.Pager('for i ({1..15}); do echo foo; sleep 1; done'));"
    "});"

    "Rat.addEventListener(['q'], function() {"
      "Rat.pop();"
    "});"
  );
}

void rat_cleanup() {
  // TODO: clean up everything else

  poll_registry_cleanup();
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

void rat_run() {
  int n;
  size_t i;

  PollItems *pis;
  struct pollfd *pfd;

  while (!done) {
    render();

    pis = poll_registry_poll_items();
    pfd = poll_registry_build_pfd(pis);

  start_poll:
    n = poll(pfd, pis->len, -1);

    fprintf(stderr, "poll returned %d\n", n);

    if (n == -1) {
      if (errno == EINTR) {
        goto start_poll;
      }

      perror("poll");
      exit(EXIT_FAILURE);
    }

    /* The first poll fd is the terminal. If user input is present, handle it
     * by itself and restart the loop to avoid conflicts with the other fd's */
    if (pfd[0].revents & POLLIN) {
      fprintf(stderr, "got tty input\n");
      handle_input(getch());
    } else {
      for (i = 1; i < pis->len; i++) {
        switch (pis->items[i]->type) {
          case PI_BUFFER_READ:
            if (pfd[i].revents & POLLHUP) {
              fprintf(stderr, "POLLHUP ready for buffer %ld\n", i);
              buffer_read_all(pis->items[i]->ptr);
            } else if (pfd[i].revents & POLLIN) {
              fprintf(stderr, "POLLIN ready for buffer %ld\n", i);
              buffer_read(pis->items[i]->ptr);
            }
            break;

          case PI_ANNOTATOR_WRITE:
            if (pfd[i].revents & POLLOUT) {
              annotator_write(pis->items[i]->ptr);
            }
            break;

          case PI_ANNOTATOR_READ:
            if (pfd[i].revents & POLLHUP) {
              fprintf(stderr, "POLLHUP ready for annotator read %ld\n", i);
              annotator_read_all(pis->items[i]->ptr);
            } else if (pfd[i].revents & POLLIN) {
              fprintf(stderr, "POLLIN ready for annotator read %ld\n", i);
              annotator_read(pis->items[i]->ptr);
            }
            break;

          case PI_USER_INPUT:
            /* shouldn't get here */
            break;
        }
      }
    }

    free_poll_items(pis);
    free(pfd);
  }
}
