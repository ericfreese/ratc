#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "annotation_parser.h"
#include "read_queue.h"

struct annotation_parser {
  ReadQueue *read_queue;
  const char *annotation_type;
  unsigned char version;
  int has_version;
};

AnnotationParser *new_annotation_parser(const char *annotation_type) {
  AnnotationParser *ap = malloc(sizeof(*ap));

  ap->version = 0;
  ap->has_version = 0;
  ap->read_queue = new_read_queue();
  ap->annotation_type = strdup(annotation_type);

  return ap;
}

void free_annotation_parser(AnnotationParser *ap) {
  free_read_queue(ap->read_queue);
  free((char*)ap->annotation_type);
  free(ap);
}

void annotation_parser_write(AnnotationParser *ap, const char *buf, size_t len) {
  read_queue_write(ap->read_queue, buf, len);
}

Annotation *annotation_parser_read(AnnotationParser *ap) {
  size_t n;
  unsigned char version;
  uint64_t num[3];
  char *val;
  Annotation *a;

  if (!ap->has_version) {
    if ((n = read_queue_read(ap->read_queue, &version, 1)) < 1) {
      goto not_enough_input;
    };

    if (version != 1) {
      fprintf(stderr, "invalid annotator version: %d\n", version);
      exit(EXIT_FAILURE);
    }

    ap->version = version;
    ap->has_version = 1;
  }

  if ((n = read_queue_read(ap->read_queue, &num, 3 * sizeof(*num))) < 3 * sizeof(*num)) {
    goto not_enough_input;
  };

  val = malloc(num[2] * sizeof(*val) + 1);

  if ((n = read_queue_read(ap->read_queue, val, num[2])) < num[2]) {
    free(val);
    goto not_enough_input;
  }

  val[n] = '\0';

  read_queue_commit(ap->read_queue);
  a = new_annotation(num[0], num[1], ap->annotation_type, val);
  free(val);
  return a;

not_enough_input:
  read_queue_rollback(ap->read_queue);
  return NULL;
}
