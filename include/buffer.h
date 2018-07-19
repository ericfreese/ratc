#ifndef BUFFER_H
#define BUFFER_H

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "read_queue.h"

typedef struct {
  uint32_t start;
  uint32_t end;
  char *type;
  char *value;
  int refs;
} Annotation;

Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value);
void free_annotation(Annotation *a);

typedef struct {
  Annotation **items;
  size_t len;
  size_t size;
} Annotations;

Annotations *new_annotations();
void free_annotations(Annotations *as);
void annotations_add(Annotations *as, Annotation *a);
Annotations *annotations_intersecting(Annotations *as, uint32_t start, uint32_t end);

typedef struct {
  ReadQueue *read_queue;
  char *annotation_type;
  unsigned char version;
  int has_version;
} AnnotationParser;

AnnotationParser *new_annotation_parser();
void free_annotation_parser(AnnotationParser *ap);
void annotation_parser_write(AnnotationParser *ap, char *buf, size_t len);
Annotation *annotation_parser_read(AnnotationParser *ap);

typedef struct {
  size_t *offsets;
  size_t len;
  size_t size;
} LineEnds;

LineEnds *new_line_ends();
void free_line_ends(LineEnds *le);
void push_line_end(LineEnds *le, size_t offset);

typedef enum {
  TK_NONE = 0,
  TK_CONTENT,
  TK_NEWLINE,
  TK_TERMSTYLE,
} TokenType;

typedef struct {
  TokenType type;
  char *value;
} Token;

void free_token(Token *t);

typedef struct {
  ReadQueue *rq;
} Tokenizer;

Tokenizer *new_tokenizer();
void free_tokenizer(Tokenizer *tr);
void tokenizer_write(Tokenizer *tr, char *buf, size_t len);
Token *tokenizer_read(Tokenizer *tr);

typedef struct {
  pid_t pid;
  int fd;
  int is_running;

  char *stream_str;
  size_t stream_len;
  FILE *stream;

  LineEnds *line_ends;
  Tokenizer *tokenizer;
  Annotations *annotations;
} Buffer;

Buffer *new_buffer(pid_t pid, int fd);
void free_buffer(Buffer *b);
ssize_t buffer_read(Buffer *b);
void buffer_read_all(Buffer *b);
char **get_buffer_lines(Buffer *b, size_t start, size_t num);
void buffer_list_annotations(Buffer *b);

typedef struct {
  Buffer *buffer;
  AnnotationParser *parser;
  size_t woffset;

  pid_t pid;
  int wfd;
  int rfd;
} Annotator;

Annotator *new_annotator(Buffer *b, char *cmd, char *annotation_type);
void annotator_write(Annotator *ar);
ssize_t annotator_read(Annotator *ar);
void annotator_read_all(Annotator *ar);
void kill_annotator(Annotator *ar);
void free_annotator(Annotator *ar);

#endif
