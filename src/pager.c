#include "rat.h"

Pager *new_pager(char *cmd) {
  Pager *p = (Pager*)malloc(sizeof(*p));

  p->cmd = cmd;

  run_pager_command(p);

  return p;
}

void free_pager(Pager *p) {
  if (p->buffer) {
    free_buffer(p->buffer);
  }

  free(p);
}

void render_pager(Pager *p) {
  char **buffer_lines;

  pthread_mutex_lock(&p->buffer->lock);
  buffer_lines = get_buffer_lines(p->buffer, 0, 10);
  pthread_mutex_unlock(&p->buffer->lock);

  for (int y = 0; buffer_lines[y] != NULL; y++) {
    mvprintw(y, 0, "%s", buffer_lines[y]);
    free(buffer_lines[y]);
  }

  free(buffer_lines);

  refresh();
}

void run_pager_command(Pager *p) {
  int fds[2];
  pid_t pid;

  pipe(fds);

  switch (pid = fork()) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      dup2(fds[1], STDOUT_FILENO);
      dup2(fds[1], STDERR_FILENO);
      close(fds[0]);
      close(fds[1]);
      execl("/bin/zsh", "zsh", "-c", p->cmd);
      perror("execl");
      exit(EXIT_FAILURE);
    default:
      close(fds[1]);
      p->buffer = new_buffer(pid, fds[0]);
  }
}
