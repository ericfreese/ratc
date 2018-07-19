#include "buffer.h"

Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value) {
  Annotation *a = malloc(sizeof(*a));

  a->start = start;
  a->end = end;
  a->type = type;
  a->value = value;
  a->refs = 1;

  return a;
}

void free_annotation(Annotation *a) {
  free(a->type);
  free(a->value);
  free(a);
}
