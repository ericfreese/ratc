#include <stdlib.h>
#include <string.h>

#include "esc_seq.h"

EscSeqPart *new_esc_seq_part(EscSeqPartType type, const char *value) {
  EscSeqPart *esp = malloc(sizeof *esp);

  esp->next = NULL;
  esp->type = type;
  esp->value = strdup(value);

  return esp;
}

void free_esc_seq_part(EscSeqPart *esp) {
  free((char*)esp->value);
  free(esp);
}

EscSeq *new_esc_seq() {
  EscSeq *es = malloc(sizeof *es);

  es->first = NULL;
  es->last = NULL;

  return es;
}

void free_esc_seq(EscSeq *es) {
  EscSeqPart *next;

  for (EscSeqPart *cursor = es->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free_esc_seq_part(cursor);
  }

  free(es);
}

void esc_seq_add_part(EscSeq *es, EscSeqPart *esp) {
  if (es->last != NULL) {
    es->last->next = esp;
    es->last = esp;
  } else {
    es->first = es->last = esp;
  }
}
