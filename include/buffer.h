#ifndef BUFFER_H
#define BUFFER_H

#include <locale.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>

#include "line_ends.h"
#include "tokenizer.h"

typedef struct {
  pid_t pid;
  int fd;
  int is_running;

  char *stream_str;
  size_t stream_len;
  FILE *stream;

  LineEnds *line_ends;
  Tokenizer *tr;
} Buffer;

Buffer *new_buffer(pid_t pid, int fd);
void free_buffer(Buffer *b);
int buffer_read(Buffer *b);
void buffer_read_all(Buffer *b);
char **get_buffer_lines(Buffer *b, size_t start, size_t num);

#endif
