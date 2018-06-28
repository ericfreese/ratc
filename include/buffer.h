#ifndef BUFFER_H
#define BUFFER_H

#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/wait.h>

#include "line_ends.h"
#include "stream.h"
#include "tokenizer.h"

typedef struct {
  pid_t pid;
  int fd;
  Stream *stream;
  LineEnds *line_ends;

  pthread_t fill_thread;
  pthread_mutex_t lock;
} Buffer;

Buffer *new_buffer(pid_t pid, int fd);
void free_buffer(Buffer *b);
void *fill_buffer(void *bptr);
char **get_buffer_lines(Buffer *b, size_t start, size_t num);

#endif
