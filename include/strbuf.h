#pragma once

#include <stdlib.h>
#include <string.h>

typedef struct {
  char *str;
  size_t len;
  size_t size;
} Strbuf;

Strbuf *new_strbuf(char *str);
void free_strbuf(Strbuf *strbuf);
void strbuf_write(Strbuf *strbuf, char *str);
