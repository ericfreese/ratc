#ifndef STREAM_H
#define STREAM_H

#include <pthread.h>
#include "strbuf.h"

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

#endif
