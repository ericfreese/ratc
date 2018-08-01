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

typedef struct annotation Annotation;
Annotation *new_annotation(uint32_t start, uint32_t end, char *type, char *value);
void annotation_ref_inc(Annotation *a);
void annotation_ref_dec(Annotation *a);
uint32_t annotation_start(Annotation *a);
uint32_t annotation_end(Annotation *a);
const char* annotation_type(Annotation *a);
const char* annotation_value(Annotation *a);

typedef struct annotations Annotations;
Annotations *new_annotations();
void free_annotations(Annotations *as);
void annotations_add(Annotations *as, Annotation *a);
Annotations *annotations_intersecting(Annotations *as, uint32_t start, uint32_t end);

typedef struct annotation_parser AnnotationParser;
AnnotationParser *new_annotation_parser();
void free_annotation_parser(AnnotationParser *ap);
void annotation_parser_write(AnnotationParser *ap, char *buf, size_t len);
Annotation *annotation_parser_read(AnnotationParser *ap);

typedef enum {
  TK_NONE = 0,
  TK_CONTENT,
  TK_NEWLINE,
  TK_TERMSTYLE,
} TokenType;

typedef struct token Token;
void free_token(Token *t);
TokenType token_type(Token *t);
const char* token_value(Token *t);

typedef struct tokenizer Tokenizer;
Tokenizer *new_tokenizer();
void free_tokenizer(Tokenizer *tr);
void tokenizer_write(Tokenizer *tr, char *buf, size_t len);
Token *tokenizer_read(Tokenizer *tr);

typedef struct buffer Buffer;
Buffer *new_buffer(pid_t pid, int fd);
void free_buffer(Buffer *b);
void close_buffer(Buffer *b);
ssize_t buffer_read(Buffer *b);
void buffer_read_all(Buffer *b);
const char **get_buffer_lines(Buffer *b, size_t start, size_t num);
//void buffer_list_annotations(Buffer *b);
void buffer_add_annotation(Buffer *b, Annotation *a);
size_t buffer_len(Buffer *b);
const char* buffer_content(Buffer *b, size_t offset);
int buffer_is_running(Buffer *b);

typedef struct annotator Annotator;
Annotator *new_annotator(Buffer *b, char *cmd, char *annotation_type);
void annotator_write(Annotator *ar);
ssize_t annotator_read(Annotator *ar);
void annotator_read_all(Annotator *ar);
void kill_annotator(Annotator *ar);
void free_annotator(Annotator *ar);

#endif
