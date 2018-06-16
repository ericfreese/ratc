#include "rat.h"

KeyStack *new_key_stack() {
  KeyStack *ks = (KeyStack*)malloc(sizeof(*ks));

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

void key_stack_to_strbuf(KeyStack *ks, Strbuf *out) {
  for (KeyStackItem *cursor = ks->first; cursor != NULL; cursor = cursor->next) {
    strbuf_write(out, (char*)cursor->key);
  }
}

