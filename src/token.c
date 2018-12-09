#include <stdlib.h>

#include "term_style.h"
#include "token.h"

struct token {
  TokenType type;
  const char *value;
  TermStyle *ts;
};

Token *new_content_token(const char *value) {
  Token *t = malloc(sizeof *t);

  t->type = TK_CONTENT;
  t->value = value;

  return t;
}

Token *new_newline_token(const char *value) {
  Token *t = malloc(sizeof *t);

  t->type = TK_NEWLINE;
  t->value = value;

  return t;
}

Token *new_termstyle_token(TermStyle *ts, const char *value) {
  Token *t = malloc(sizeof *t);

  t->type = TK_TERMSTYLE;
  t->ts = ts;
  t->value = value;

  return t;
}

void free_token(Token *t) {
  if (t->type == TK_CONTENT || t->type == TK_TERMSTYLE) {
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

