#include <stdlib.h>
#include <string.h>

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
  t->value = strdup(value);

  return t;
}

Token *new_newline_token(const char *value) {
  Token *t = malloc(sizeof *t);

  t->type = TK_NEWLINE;
  t->value = value;

  return t;
}

Token *new_termstyle_token(TermStyle *ts) {
  Token *t = malloc(sizeof *t);

  t->type = TK_TERMSTYLE;
  t->ts = term_style_dup(ts);

  return t;
}

void free_token(Token *t) {
  if (t->type == TK_CONTENT) {
    free((char*)t->value);
  }

  if (t->type == TK_TERMSTYLE) {
    free(t->ts);
  }

  free(t);
}

TokenType token_type(Token *t) {
  return t->type;
}

const char* token_value(Token *t) {
  return t->value;
}

TermStyle *token_termstyle(Token *t) {
  return t->ts;
}
