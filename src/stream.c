#include "stream.h"

Stream *new_stream() {
  Stream *stream = (Stream*)malloc(sizeof(Stream));

  stream->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  stream->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

  stream->strbuf = new_strbuf("");

  return stream;
}

void free_stream(Stream *stream) {
  pthread_mutex_destroy(&stream->lock);
  pthread_cond_destroy(&stream->cond);

  free_strbuf(stream->strbuf);

  free(stream);
}

void stream_write(Stream *stream, char *buf) {
  pthread_mutex_lock(&stream->lock);

  strbuf_write(stream->strbuf, buf);
  pthread_cond_broadcast(&stream->cond);

  pthread_mutex_unlock(&stream->lock);
}

size_t stream_close(Stream *stream) {
  pthread_mutex_lock(&stream->lock);

  stream->closed = 1;
  pthread_cond_broadcast(&stream->cond);

  pthread_mutex_unlock(&stream->lock);

  return 0;
}

StreamReader *new_stream_reader(Stream *stream) {
  StreamReader *sr = (StreamReader*)malloc(sizeof(StreamReader));

  sr->stream = stream;
  sr->offset = 0;

  return sr;
}

void free_stream_reader(StreamReader *sr) {
  free(sr);
}

// TODO: Allow multiple simultaneous readers
size_t stream_reader_read(StreamReader *sr, char *buf, size_t nbyte) {
  size_t ret;

  pthread_mutex_lock(&sr->stream->lock);

  while (!sr->stream->closed && sr->offset + nbyte > sr->stream->strbuf->len) {
    pthread_cond_wait(&sr->stream->cond, &sr->stream->lock);
  }

  if (sr->stream->closed) {
    if (sr->offset < sr->stream->strbuf->len - 1) {
      ret = sr->stream->strbuf->len - sr->offset;
      memcpy(buf, &sr->stream->strbuf->str[sr->offset], ret);
    } else {
      ret = 0;
    }
  } else {
    memcpy(buf, &sr->stream->strbuf->str[sr->offset], nbyte);
    ret = nbyte;
  }

  sr->offset += ret;

  pthread_mutex_unlock(&sr->stream->lock);

  return ret;
}

