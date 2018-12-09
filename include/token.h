#pragma once

#include "term_style.h"

typedef enum {
  TK_NONE = 0,
  TK_CONTENT,
  TK_NEWLINE,
  TK_TERMSTYLE,
} TokenType;

typedef struct token Token;
Token *new_content_token(const char *value);
Token *new_newline_token(const char *value);
Token *new_termstyle_token(TermStyle *ts, const char *value);
void free_token(Token *t);
TokenType token_type(Token *t);
const char* token_value(Token *t);
