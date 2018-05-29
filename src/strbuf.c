#include "rat.h"

Strbuf *new_strbuf(char *str) {
  size_t len = strlen(str);
  Strbuf *strbuf = (Strbuf*)malloc(sizeof(Strbuf));

  strbuf->size = 8;

  while (strbuf->size < len + 1) {
    strbuf->size *= 2;
  }

  strbuf->len = len;
  strbuf->str = (char*)malloc(strbuf->size);
  memcpy(strbuf->str, str, len + 1);

  return strbuf;
}

void free_strbuf(Strbuf *strbuf) {
  free(strbuf->str);
  free(strbuf);
}

void strbuf_write(Strbuf *strbuf, char *str) {
  size_t extra = strlen(str);
  size_t new_size = strbuf->size;

  while (new_size < strbuf->len + extra + 1) {
    new_size *= 2;
  }

  if (new_size > strbuf->size) {
    strbuf->size = new_size;
    strbuf->str = (char*)realloc(strbuf->str, strbuf->size);
  }

  strbuf->len += extra;
  strncat(strbuf->str, str, extra);
}

