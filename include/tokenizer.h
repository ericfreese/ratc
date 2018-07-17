#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "read_queue.h"

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
void tokenizer_buffer_input(Tokenizer *tr, char *buf, size_t len);
Token *read_token(Tokenizer *tr);

#endif
