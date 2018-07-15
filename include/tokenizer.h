#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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
  // TODO: The memstream hangs on to old data even after tokens have been read
  // from it. Replace with some data structure that discards the data after
  // it's been successfully tokenized (See AnnotationParser)
  char *read_buffer_str;
  size_t read_buffer_len;
  FILE *read_buffer;

  // To seek back to if we get to a place where we need more input for a complete token
  long last_offset;
} Tokenizer;

Tokenizer *new_tokenizer();
void free_tokenizer(Tokenizer *tr);
void tokenizer_buffer_input(Tokenizer *tr, char *str, size_t len);
Token *read_token(Tokenizer *tr);

#endif
