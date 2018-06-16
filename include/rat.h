#ifndef RAT_H
#define RAT_H

#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

typedef struct InputQueueItem InputQueueItem;
struct InputQueueItem {
  int ch;
  InputQueueItem *next;
};

typedef struct {
  InputQueueItem *first;
  InputQueueItem *last;
} InputQueue;

InputQueue *new_input_queue();
void enqueue_input(InputQueue *q, int ch);
int dequeue_input(InputQueue *q);

typedef struct {
  char *str;
  size_t len;
  size_t size;
} Strbuf;

Strbuf *new_strbuf(char *str);
void free_strbuf(Strbuf *strbuf);
void strbuf_write(Strbuf *strbuf, char *str);

typedef struct {
  Strbuf *strbuf;
  int closed;
  pthread_cond_t cond;
  pthread_mutex_t lock;
} Stream;

Stream *new_stream();
void free_stream(Stream *stream);
void stream_write(Stream *stream, char *buf);
size_t stream_close(Stream *stream);

typedef struct {
  Stream *stream;
  size_t offset;
} StreamReader;

StreamReader *new_stream_reader(Stream *stream);
void free_stream_reader(StreamReader *sr);
size_t stream_reader_read(StreamReader *sr, char *buf, size_t nbyte);

typedef struct {
  size_t *offsets;
  size_t len;
  size_t size;
} LineEnds;

LineEnds *new_line_ends();
void free_line_ends(LineEnds *le);
void push_line_end(LineEnds *le, size_t offset);

typedef struct {
  int fd;
  Stream *stream;
  LineEnds *line_ends;

  pthread_t fill_thread;
  pthread_mutex_t lock;
} Buffer;

Buffer *new_buffer(int fd);
void free_buffer(Buffer *b);
void *fill_buffer(void *bptr);

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
