#include "buffer.h"

Annotations *new_annotations() {
  Annotations *as = malloc(sizeof(*as));

  as->len = 0;
  as->size = 8;
  as->items = malloc(as->size * sizeof(*as->items));

  return as;
}

void free_annotations(Annotations *as) {
  for (size_t i = 0; i < as->len; i++) {
    as->items[i]->refs--;

    if (as->items[i]->refs == 0) {
      free_annotation(as->items[i]);
    }
  }

  free(as->items);
  free(as);
}

void annotations_add(Annotations *as, Annotation *a) {
  if (as->len == as->size) {
    as->size *= 2;
    as->items = realloc(as->items, as->size * sizeof(*as->items));
  }

  a->refs++;
  as->items[as->len] = a;
  as->len++;
}

Annotations *annotations_intersecting(Annotations *as, uint32_t start, uint32_t end) {
  Annotations *asi = new_annotations();

  for (size_t i = 0; i < as->len; i++) {
    if (as->items[i]->start < end && as->items[i]->end > start) {
      annotations_add(asi, as->items[i]);
    }
  }

  return asi;
}
