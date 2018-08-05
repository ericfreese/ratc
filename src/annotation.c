#include <string.h>

#include "annotation.h"
#include "refs.h"

struct annotation {
  uint32_t start;
  uint32_t end;
  const char *type;
  const char *value;
  struct refs refs;
};

void free_annotation(const struct refs *r) {
  Annotation *a = container_of(r, Annotation, refs);

  free((char*)a->type);
  free((char*)a->value);
  free(a);
}

Annotation *new_annotation(uint32_t start, uint32_t end, const char *type, const char *value) {
  Annotation *a = malloc(sizeof(*a));

  a->start = start;
  a->end = end;
  a->type = strdup(type);
  a->value = strdup(value);
  a->refs = (struct refs){free_annotation, 1};

  return a;
}

void annotation_ref_inc(Annotation *a) {
  ref_inc(&a->refs);
}

void annotation_ref_dec(Annotation *a) {
  ref_dec(&a->refs);
}

uint32_t annotation_start(Annotation *a) {
  return a->start;
}

uint32_t annotation_end(Annotation *a) {
  return a->end;
}

const char* annotation_type(Annotation *a) {
  return a->type;
}

const char* annotation_value(Annotation *a) {
  return a->value;
}
