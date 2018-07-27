#include "key_stack.h"

KeyStack *new_key_stack() {
  KeyStack *ks = (KeyStack*)malloc(sizeof(*ks));

  ks->first = NULL;
  ks->last = NULL;

  return ks;
}

void key_stack_push(KeyStack *ks, char *key) {
  KeyStackItem *item = (KeyStackItem*)malloc(sizeof(*item));

  item->key = key;
  item->next = NULL;

  if (ks->last != NULL) {
    ks->last->next = item;
    ks->last = item;
  } else {
    ks->first = item;
    ks->last = item;
  }
}

char *stringify_key_stack(KeyStack *ks) {
  char *buf;
  size_t len;
  FILE *stream = open_memstream(&buf, &len);

  for (KeyStackItem *cursor = ks->first; cursor != NULL; cursor = cursor->next) {
    fputs(cursor->key, stream);
  }

  fclose(stream);

  return buf;
}
