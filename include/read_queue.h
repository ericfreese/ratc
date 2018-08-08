#pragma once

#include <stdlib.h>
#include <string.h>

typedef struct read_queue ReadQueue;
ReadQueue *new_read_queue();
void free_read_queue(ReadQueue *rq);
void read_queue_write(ReadQueue *rq, const void *buf, size_t len);
size_t read_queue_read(ReadQueue *rq, void *buf, size_t len);
void read_queue_rollback(ReadQueue *rq);
void read_queue_commit(ReadQueue *rq);
