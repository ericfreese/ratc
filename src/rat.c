#include "rat.h"

void main_loop() {
  int done = 0;
  int retval;
  KeyStack *key_stack = new_key_stack();

  struct pollfd pfds[1];
  pfds[0].fd = open("/dev/tty", O_RDONLY);
  pfds[0].events = POLLIN;

  Pager *p = new_pager("git diff --no-color");

  while (!done) {
    retval = poll(pfds, 1, 100);

    if (retval == -1) {
      // Handle interrupted syscall
      if (errno == EINTR) {
        continue;
      }

      perror("poll");
      exit(EXIT_FAILURE);
    } else if (retval && pfds[0].revents & POLLIN) {
      int ch = getch();
      key_stack_push(key_stack, (char*)keyname(ch));

      Strbuf *sb = new_strbuf("");
      key_stack_to_strbuf(key_stack, sb);
      fprintf(stderr, "keystack: '%s'\n", sb->str);
      free_strbuf(sb);

      // TODO: Handle key event
    }

    render_pager(p);
  }
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();

  main_loop();

  endwin();

  return EXIT_SUCCESS;
}

