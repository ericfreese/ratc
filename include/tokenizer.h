#pragma once

#include <stdlib.h>

#include "token.h"

typedef struct tokenizer Tokenizer;
Tokenizer *new_tokenizer();
void free_tokenizer(Tokenizer *tr);
void tokenizer_write(Tokenizer *tr, char *buf, size_t len);
Token *tokenizer_read(Tokenizer *tr);
