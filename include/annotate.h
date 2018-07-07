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
  Buffer *buffer;
  size_t woffset;

  pid_t pid;
  int wfd;
  int rfd;

  // TODO: Need some sort of buffer to handle reading partial annotations
  //char *content;
  //size_t content_len;
  //FILE *rbuffer;
} Annotator;

Annotator *new_annotator(Buffer *b, char *cmd);
void annotator_write(Annotator *a);
Annotation *annotator_read(Annotator *a);
void kill_annotator(Annotator *a);
void free_annotator(Annotator *a);

#endif
