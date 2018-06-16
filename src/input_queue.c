#include "rat.h"

InputQueue *new_input_queue() {
  InputQueue *q = (InputQueue*)malloc(sizeof(*q));

  q->first = q->last = NULL;

  return q;
}

void enqueue_input(InputQueue *q, int ch) {
  InputQueueItem *item = (InputQueueItem*)malloc(sizeof(*item));

  item->ch = ch;
  item->next = NULL;

  if (q->last != NULL) {
    q->last->next = item;
    q->last = item;
  } else {
    q->first = item;
    q->last = item;
  }
}

int dequeue_input(InputQueue *q) {
  InputQueueItem *first = q->first;
  int ch = first->ch;

  q->first = first->next;

  if (q->first == NULL) {
    q->last = NULL;
  }

  free(first);

  return ch;
}
