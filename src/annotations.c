#include "buffer.h"

struct annotations {
  Annotation **items;
  size_t len;
  size_t size;
};

Annotations *new_annotations() {
  Annotations *as = malloc(sizeof(*as));

  as->len = 0;
  as->size = 8;
  as->items = malloc(as->size * sizeof(*as->items));

  return as;
}

void free_annotations(Annotations *as) {
  for (size_t i = 0; i < as->len; i++) {
    annotation_ref_dec(as->items[i]);
  }

  free(as->items);
  free(as);
}

void annotations_add(Annotations *as, Annotation *a) {
  if (as->len == as->size) {
    as->size *= 2;
    as->items = realloc(as->items, as->size * sizeof(*as->items));
  }

  annotation_ref_inc(a);
  as->items[as->len] = a;
  as->len++;
}

Annotations *annotations_intersecting(Annotations *as, uint32_t start, uint32_t end) {
  Annotations *asi = new_annotations();

  for (size_t i = 0; i < as->len; i++) {
    if (annotation_start(as->items[i]) < end && annotation_end(as->items[i]) > start) {
      annotations_add(asi, as->items[i]);
    }
  }

  return asi;
}
