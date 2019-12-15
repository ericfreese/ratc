#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "key_seq.h"

typedef struct key_seq_item KeySeqItem;
struct key_seq_item {
  char *key;
  KeySeqItem *next;
};

KeySeqItem *new_key_seq_item(char *key) {
  KeySeqItem *ksi = malloc(sizeof *ksi);

  ksi->key = strdup(key);
  ksi->next = NULL;

  return ksi;
}

void free_key_seq_item(KeySeqItem *ksi) {
  free(ksi->key);
  free(ksi);
}

struct key_seq {
  KeySeqItem *first;
  KeySeqItem *last;
  size_t len;
};

KeySeq *new_key_seq() {
  KeySeq *ks = malloc(sizeof *ks);

  ks->first = NULL;
  ks->last = NULL;
  ks->len = 0;

  return ks;
}

void free_key_seq(KeySeq *ks) {
  KeySeqItem *next;

  for (KeySeqItem *cursor = ks->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free_key_seq_item(cursor);
  }

  free(ks);
}

void key_seq_add(KeySeq *ks, char *key) {
  KeySeqItem *item = new_key_seq_item(key);

  if (ks->last == NULL) {
    ks->first = item;
    ks->last = item;
  } else {
    ks->last->next = item;
    ks->last = item;
  }

  ks->len++;
}

int key_seq_ends_with(KeySeq *ks, KeySeq *end) {
  if (ks->last == NULL || end->last == NULL) {
    return 0;
  }

  // TODO: Check more than just the last

  return !strcmp(ks->last->key, end->last->key);
}

char *key_seq_str(KeySeq *ks) {
  char *buf;
  size_t len;
  FILE *stream = open_memstream(&buf, &len);

  for (KeySeqItem *cursor = ks->first; cursor != NULL; cursor = cursor->next) {
    fputs(cursor->key, stream);
  }

  fclose(stream);

  return buf;
}
