#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>

#include "pager.h"

struct pager {
  char *cmd;
  Buffer *buffer;
  int scroll;
  int cursor;
  Box *box;
};

Pager *new_pager(char *cmd) {
  Pager *p = (Pager*)malloc(sizeof(*p));

  p->cmd = cmd;
  p->box = new_box(3, 3, 100, 20);

  run_pager_command(p);

  return p;
}

void free_pager(Pager *p) {
  if (p->buffer) {
    free_buffer(p->buffer);
  }

  free_box(p->box);

  free(p);
}

void run_pager_command(Pager *p) {
  int fds[2];
  char* shell;
  pid_t pid;

  pipe(fds);

  switch (pid = fork()) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      shell = getenv("SHELL");

      dup2(fds[1], STDOUT_FILENO);
      dup2(fds[1], STDERR_FILENO);

      close(fds[0]);
      close(fds[1]);

      execl(shell, shell, "-c", p->cmd, NULL);
      perror("execl");
      exit(EXIT_FAILURE);
  }

  close(fds[1]);

  p->buffer = new_buffer(pid, fds[0]);
}

Buffer *get_buffer(Pager *p) {
  return p->buffer;
}

void render_pager(void *pptr) {
  Pager *p = (Pager*)pptr;

  char **buffer_lines;

  buffer_lines = get_buffer_lines(p->buffer, 0, box_height(p->box));

  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  attron(COLOR_PAIR(1));

  for (int y = 0; buffer_lines[y] != NULL; y++) {
    mvprintw(y + box_top(p->box), box_left(p->box), "%s", buffer_lines[y]);
    free(buffer_lines[y]);
  }

  attroff(COLOR_PAIR(2));

  free(buffer_lines);

  refresh();
}

void set_pager_box(void *pptr, int left, int top, int width, int height) {
  Pager *p = (Pager*)pptr;

  box_set_left(p->box, left);
  box_set_top(p->box, top);
  box_set_width(p->box, width);
  box_set_height(p->box, height);
}

Box *get_pager_box(void *pptr) {
  return ((Pager*)pptr)->box;
}

Widget *new_pager_widget(Pager *p) {
  return new_widget(p, render_pager, set_pager_box, get_pager_box);
}
