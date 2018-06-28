#ifndef PAGER_H
#define PAGER_H

#include <ncurses.h>
#include <unistd.h>
#include "buffer.h"

typedef struct {
  char *cmd;
  Buffer *buffer;
  int scroll;
  int cursor;
} Pager;

Pager *new_pager(char* cmd);
void free_pager(Pager *p);
void render_pager(Pager *p);
void run_pager_command(Pager *p);

#endif
