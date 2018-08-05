#include <stdio.h>
#include <string.h>

#include "annotator.h"
#include "refs.h"

struct annotator {
  const char* cmd;
  const char* annotation_type;
  struct refs refs;
};

void free_annotator(const struct refs *r) {
  Annotator *ar = container_of(r, Annotator, refs);

  fprintf(stderr, "FREEING ANNOTATOR %p\n", ar);

  free((char*)ar->cmd);
  free((char*)ar->annotation_type);
  free(ar);
}

Annotator *new_annotator(char *cmd, char *annotation_type) {
  Annotator *ar = malloc(sizeof(*ar));

  ar->cmd = strdup(cmd);
  ar->annotation_type = strdup(annotation_type);
  ar->refs = (struct refs){free_annotator, 1};

  return ar;
}

void annotator_ref_inc(Annotator *ar) {
  ref_inc(&ar->refs);
}

void annotator_ref_dec(Annotator *ar) {
  ref_dec(&ar->refs);
}

const char *annotator_command(Annotator *ar) {
  return ar->cmd;
}

const char *annotator_type(Annotator *ar) {
  return ar->annotation_type;
}
