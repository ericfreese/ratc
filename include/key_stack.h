#ifndef KEY_STACK_H
#define KEY_STACK_H

#include <stdlib.h>
#include "strbuf.h"

typedef struct KeyStackItem KeyStackItem;
struct KeyStackItem {
  char *key;
  KeyStackItem *next;
};

typedef struct {
  KeyStackItem *first;
  KeyStackItem *last;
} KeyStack;

KeyStack *new_key_stack();
void key_stack_push(KeyStack *ks, char *key);
void key_stack_to_strbuf(KeyStack *ks, Strbuf *out);

#endif
