#pragma once

typedef enum {
  TK_NONE = 0,
  TK_CONTENT,
  TK_NEWLINE,
  TK_TERMSTYLE,
} TokenType;

typedef struct token Token;
Token *new_token(TokenType type, const char *value);
void free_token(Token *t);
TokenType token_type(Token *t);
const char* token_value(Token *t);
