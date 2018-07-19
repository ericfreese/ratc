#ifndef ANNOTATE_H
#define ANNOTATE_H

#include <sys/wait.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include "buffer.h"

typedef struct {
  uint32_t start;
  uint32_t end;
  char *type;
  char *value;
} Annotation;

Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value);
void free_annotation(Annotation *a);

typedef struct {
  ReadQueue *read_queue;
  char *annotation_type;
  unsigned char version;
  int has_version;
} AnnotationParser;

AnnotationParser *new_annotation_parser();
void free_annotation_parser(AnnotationParser *ap);
void annotation_parser_buffer_input(AnnotationParser *ap, char *buf, size_t len);
Annotation *read_annotation(AnnotationParser *ap);

typedef struct {
  Buffer *buffer;
  AnnotationParser *parser;
  size_t woffset;

  pid_t pid;
  int wfd;
  int rfd;
} Annotator;

Annotator *new_annotator(Buffer *b, char *cmd, char *annotation_type);
void annotator_write(Annotator *a);
ssize_t annotator_read(Annotator *a);
void annotator_read_all(Annotator *a);
void kill_annotator(Annotator *a);
void free_annotator(Annotator *a);

#endif
