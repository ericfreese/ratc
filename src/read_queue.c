#include <stdio.h>

#include "read_queue.h"

ReadQueue *new_read_queue() {
  ReadQueue *rq = malloc(sizeof(*rq));

  rq->first = NULL;
  rq->last = NULL;
  rq->offset = 0;
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

void read_queue_write(ReadQueue *rq, void *buf, size_t len) {
  ReadQueueChunk *chunk = new_read_queue_chunk(buf, len);

  if (rq->last != NULL) {
    rq->last->next = chunk;
    rq->last = chunk;
  } else {
    rq->first = rq->last = chunk;
  }

  //fprintf(stderr, "wrote %ld bytes to read queue\n", len);
}

size_t read_queue_read(ReadQueue *rq, void *buf, size_t len) {
  size_t chunk_start = 0;
  size_t local_offset;
  size_t copy_amount;
  size_t n = 0;

  for (ReadQueueChunk *cursor = rq->first; cursor != NULL; cursor = cursor->next) {
    if (len == 0) {
      break;
    }

    local_offset = rq->tmp_offset - chunk_start;

    if (cursor->len > local_offset) {
      copy_amount = cursor->len - local_offset;

      if (copy_amount > len) {
        copy_amount = len;
      }

      memcpy(
        buf + n,
        cursor->buf + local_offset,
        copy_amount
      );

      len -= copy_amount;
      rq->tmp_offset += copy_amount;
      n += copy_amount;
    }

    chunk_start += cursor->len;

    break;
  }

  //fprintf(stderr, "read %ld bytes from read queue\n", n);

  return n;
}

void read_queue_rollback(ReadQueue *rq) {
  rq->tmp_offset = rq->offset;
}

void read_queue_commit(ReadQueue *rq) {
  rq->offset = rq->tmp_offset;

  size_t chopped_len = 0;

  ReadQueueChunk *cursor = rq->first;

  while (rq->first != NULL && chopped_len + cursor->len <= rq->tmp_offset) {
    chopped_len += cursor->len;

    rq->first = cursor->next;

    free_read_queue_chunk(cursor);
    cursor = rq->first;
  }

  if (rq->first == NULL) {
    rq->last = NULL;
  }

  rq->offset = rq->tmp_offset = rq->tmp_offset - chopped_len;
}

ReadQueueChunk *new_read_queue_chunk(void *buf, size_t len) {
  ReadQueueChunk *rqc = malloc(sizeof(*rqc));

  rqc->buf = buf;
  rqc->len = len;
  rqc->next = NULL;

  return rqc;
}

void free_read_queue_chunk(ReadQueueChunk *rqc) {
  free(rqc->buf);
  free(rqc);
}
