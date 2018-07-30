#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <poll.h>
#include <ncurses.h>

#include "rat.h"
#include "duktape.h"

#include "buffer.h"
#include "key_stack.h"
#include "poll_registry.h"
#include "js_api.h"

void main_loop(duk_context *duk_ctx) {
  int done = 0;
  int n, i;
  KeyStack *key_stack = new_key_stack();

  PollItems *pis;
  struct pollfd *pfd;

  //Pager *p = new_pager("i=1; while true; do sleep 3; echo foo $i; i=$((i + 1)); done");
  //Pager *p = new_pager("for i ({1..15}); do cat src/buffer.c; done");
  //Pager *p = new_pager("for i ({1..15}); do echo foo; sleep 1; done");
  //Widget *pw = new_pager_widget(p);

  while (!done) {
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
      int ch = getch();
      key_stack_push(key_stack, (char*)keyname(ch));

      switch (ch) {
        case 'Q':
          done = 1;
          break;
        case 'q':
          //rat_pop_pager();
          //break;
        case 'p':
          //duk_eval_string_noresult(duk_ctx,
          //  "Rat.push(new Rat.Pager('for i ({1..15}); do echo foo; sleep 1; done'));"
          //);
          break;
        case 'a':
          // new_annotator(p->buffer, "stdbuf -oL -eL sed -e 's/^/annotator: /' >> /dev/null");
          //new_annotator(
          //  p->buffer,
          //  "while read LINE; do echo \"annotator: $LINE\" >> /dev/null; sleep 0.01; done",
          //  "foo.bar"
          //);

          //new_annotator(
          //  get_buffer(p),
          //  "./test-annotator foo | tee >(xxd >> debug.log)",
          //  "regex"
          //);
          break;
        case 'l':
          //buffer_list_annotations(get_buffer(p));
          break;
      }

      char *kstr = stringify_key_stack(key_stack);
      fprintf(stderr, "keystack: '%s'\n", kstr);
      free(kstr);

      // TODO: Handle key event

      rat_handle_input(ch);
    } else {
      for (i = 1; i < pis->len; i++) {
        switch (pis->items[i]->type) {
          case PI_BUFFER_READ:
            if (pfd[i].revents & POLLHUP) {
              fprintf(stderr, "POLLHUP ready for buffer %d\n", i);
              buffer_read_all(pis->items[i]->ptr);
            } else if (pfd[i].revents & POLLIN) {
              fprintf(stderr, "POLLIN ready for buffer %d\n", i);
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
              fprintf(stderr, "POLLHUP ready for annotator read %d\n", i);
              annotator_read_all(pis->items[i]->ptr);
            } else if (pfd[i].revents & POLLIN) {
              fprintf(stderr, "POLLIN ready for annotator read %d\n", i);
              annotator_read(pis->items[i]->ptr);
            }
            break;

          case PI_USER_INPUT:
            /* shouldn't get here */
            break;
        }
      }
    }

    erase();
    rat_render();
    refresh();

    free_poll_items(pis);
    free(pfd);
  }
}

static void duk_fatal_handler(void *udata, const char *msg) {
  fprintf(stderr, "FATAL: %s\n", msg);
  abort();
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  start_color();

  rat_init();

  duk_context *duk_ctx = duk_create_heap(NULL, NULL, NULL, NULL, duk_fatal_handler);
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

  main_loop(duk_ctx);

  poll_registry_cleanup();

  duk_destroy_heap(duk_ctx);

  rat_cleanup();

  endwin();

  return EXIT_SUCCESS;
}

