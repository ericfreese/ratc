#include <stdio.h>
#include <string.h>

#include "read_queue.h"

typedef struct read_queue_chunk ReadQueueChunk;
struct read_queue_chunk {
  const void *buf;
  size_t len;
  ReadQueueChunk *next;
};

ReadQueueChunk *new_read_queue_chunk(const void *buf, size_t len) {
  ReadQueueChunk *rqc = malloc(sizeof(*rqc));

  rqc->buf = malloc(len);
  memcpy((char*)rqc->buf, buf, len);

  rqc->len = len;
  rqc->next = NULL;

  return rqc;
}

void free_read_queue_chunk(ReadQueueChunk *rqc) {
  free((void*)rqc->buf);
  free(rqc);
}

struct read_queue {
  ReadQueueChunk *first;
  ReadQueueChunk *last;
  size_t offset;
  ReadQueueChunk *tmp_first;
  size_t tmp_offset;
};

ReadQueue *new_read_queue() {
  ReadQueue *rq = malloc(sizeof(*rq));

  rq->first = NULL;
  rq->last = NULL;
  rq->offset = 0;
  rq->tmp_first = NULL;
  rq->tmp_offset = 0;

  return rq;
}

void free_read_queue(ReadQueue *rq) {
  ReadQueueChunk *next;

  for (ReadQueueChunk *cursor = rq->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free_read_queue_chunk(cursor);
  }

  free(rq);
}

int read_queue_readable(ReadQueue *rq) {
  return rq->tmp_first != NULL;
}

void read_queue_write(ReadQueue *rq, const void *buf, size_t len) {
  if (len == 0) {
    return;
  }

  ReadQueueChunk *chunk = new_read_queue_chunk(buf, len);

  if (rq->last != NULL) {
    rq->last->next = chunk;
    rq->last = chunk;
  } else {
    rq->first = rq->last = chunk;
  }

  if (rq->tmp_first == NULL) {
    rq->tmp_first = chunk;
  }
}

char read_queue_peek(ReadQueue *rq) {
  if (rq->tmp_first == NULL) {
    return '\0';
  }

  return ((char*)rq->tmp_first->buf)[rq->tmp_offset];
}

size_t read_queue_read(ReadQueue *rq, void *buf, size_t len) {
  size_t remaining_in_chunk;
  size_t copy_amount;
  size_t n = 0;

  while (len && rq->tmp_first != NULL) {
    remaining_in_chunk = rq->tmp_first->len - rq->tmp_offset;

    if (len < remaining_in_chunk) {
      copy_amount = len;
    } else {
      copy_amount = remaining_in_chunk;
    }

    memcpy(buf + n, rq->tmp_first->buf + rq->tmp_offset, copy_amount);
    n += copy_amount;
    len -= copy_amount;
    rq->tmp_offset += copy_amount;

    if (rq->tmp_offset == rq->tmp_first->len) {
      rq->tmp_first = rq->tmp_first->next;
      rq->tmp_offset = 0;
    }
  }

  return n;
}

size_t read_queue_advance(ReadQueue *rq, size_t n) {
  size_t new_offset = rq->tmp_offset + n;
  ReadQueueChunk *new_tmp_first = rq->tmp_first;

  while (new_tmp_first != NULL && new_offset >= new_tmp_first->len) {
    new_offset -= new_tmp_first->len;
    new_tmp_first = new_tmp_first->next;
  }

  if (new_tmp_first == NULL && new_offset > 0) {
    return 0;
  }

  rq->tmp_offset = new_offset;
  rq->tmp_first = new_tmp_first;

  return 1;
}

void read_queue_rollback(ReadQueue *rq) {
  rq->tmp_first = rq->first;
  rq->tmp_offset = rq->offset;
}

void read_queue_commit(ReadQueue *rq) {
  ReadQueueChunk *tmp;

  while (rq->first != rq->tmp_first) {
    tmp = rq->first;
    rq->first = rq->first->next;

    free_read_queue_chunk(tmp);
  }

  if (rq->first == NULL) {
    rq->last = NULL;
  }

  rq->offset = rq->tmp_offset;
}
