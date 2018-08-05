#pragma once

#include <stdlib.h>
#include <string.h>

typedef struct ReadQueueChunk ReadQueueChunk;
struct ReadQueueChunk {
  const void *buf;
  size_t len;
  ReadQueueChunk *next;
};

typedef struct {
  ReadQueueChunk *first;
  ReadQueueChunk *last;
  size_t offset;
  size_t tmp_offset;
} ReadQueue;

ReadQueueChunk *new_read_queue_chunk(const void *buf, size_t len);
void free_read_queue_chunk(ReadQueueChunk *rq);

ReadQueue *new_read_queue();
void free_read_queue(ReadQueue *rq);
void read_queue_write(ReadQueue *rq, const void *buf, size_t len);
size_t read_queue_read(ReadQueue *rq, void *buf, size_t len);
void read_queue_rollback(ReadQueue *rq);
void read_queue_commit(ReadQueue *rq);
