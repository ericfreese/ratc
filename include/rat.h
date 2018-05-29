#ifndef RAT_H
#define RAT_H

#include <fcntl.h>
#include <locale.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct strbuf {
  char *str;
  size_t len;
  size_t size;
} Strbuf;

Strbuf *new_strbuf(char *str);
void free_strbuf(Strbuf *strbuf);
void strbuf_write(Strbuf *strbuf, char *str);

typedef struct stream {
  Strbuf *strbuf;
  int closed;
  pthread_cond_t cond;
  pthread_mutex_t lock;
} Stream;

Stream *new_stream();
void free_stream(Stream *stream);
void stream_write(Stream *stream, char *buf);
size_t stream_close(Stream *stream);

typedef struct stream_reader {
  Stream *stream;
  size_t offset;
} StreamReader;

StreamReader *new_stream_reader(Stream *stream);
void free_stream_reader(StreamReader *sr);
ssize_t stream_reader_read(StreamReader *sr, char *buf, size_t nbyte);

typedef struct buffer {
  Stream *stream;
  pthread_mutex_t lock;
} Buffer;

#endif

