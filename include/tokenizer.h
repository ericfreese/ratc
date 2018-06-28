#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <unistd.h>
#include "strbuf.h"

typedef enum {
  TK_NONE = 0,
  TK_CONTENT,
  TK_NEWLINE,
  TK_TERMSTYLE,
} TokenType;

typedef struct {
  TokenType type;
  Strbuf *value;
} Token;

typedef struct {
  int fd;
  char unread_byte;
  int was_unread;
} Tokenizer;

Tokenizer *new_tokenizer(int fd);
void free_tokenizer(Tokenizer *tr);
ssize_t read_byte(Tokenizer *tr, char *byte);
int unread_byte(Tokenizer *tr, char *byte);
ssize_t read_token(Tokenizer *tr, Token *t);

#endif
