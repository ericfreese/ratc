#include <stdlib.h>

#include "token.h"

struct token {
  TokenType type;
  const char *value;
};

Token *new_token(TokenType type, const char *value) {
  Token *t = malloc(sizeof *t);

  t->type = type;
  t->value = value;

  return t;
}

void free_token(Token *t) {
  if (t->type == TK_CONTENT) {
    free((char*)t->value);
  }

  free(t);
}

TokenType token_type(Token *t) {
  return t->type;
}

const char* token_value(Token *t) {
  return t->value;
}

