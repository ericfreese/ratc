#ifndef KEY_STACK_H
#define KEY_STACK_H

#include <stdlib.h>
#include <stdio.h>
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
char *stringify_key_stack(KeyStack *ks);

#endif
