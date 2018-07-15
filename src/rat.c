#include "rat.h"

void main_loop() {
  int done = 0;
  int n, i;
  KeyStack *key_stack = new_key_stack();

  PollItems *pis;
  struct pollfd *pfd;

  poll_registry_add(PI_USER_INPUT, NULL, open("/dev/tty", O_RDONLY));

  //Pager *p = new_pager("i=1; while true; do sleep 1; echo foo $i; i=$((i + 1)); done");
  Pager *p = new_pager("git diff --no-color");

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

      if (ch == 'a') {
        Annotator *a = new_annotator(p->buffer, "stdbuf -oL -eL sed -e 's/^/annotator: /' >> debug.log");
        poll_registry_add(PI_ANNOTATOR_WRITE, a, a->wfd);
        // poll_registry_add(PI_ANNOTATOR_READ, a, a->rfd);
      }

      char *kstr = stringify_key_stack(key_stack);
      fprintf(stderr, "keystack: '%s'\n", kstr);
      free(kstr);

      // TODO: Handle key event
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
            annotator_write(pis->items[i]->ptr);
            break;

          case PI_ANNOTATOR_READ:
      //      // Read annotations (or partial annotations) from annotators who have data for us
      //      if (pfds[3].revents & POLLIN) {
      //        Annotation *ann = annotator_read(a);
      //        fprintf(stderr, "Got annotation { start: %d, end: %d, type: %s, value: %s }\n", ann->start, ann->end, ann->type, ann->value);
      //        free_annotation(ann);
      //        // TODO: Add annotation to buffer
      //      }
            break;

          case PI_USER_INPUT:
            /* shouldn't get here */
            break;
        }
      }
    }

    render_pager(p);

    free_poll_items(pis);
    free(pfd);
  }
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();

  poll_registry_init();

  main_loop();

  poll_registry_cleanup();

  endwin();

  return EXIT_SUCCESS;
}

