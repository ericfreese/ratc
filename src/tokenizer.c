#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "buffer.h"

struct token {
  TokenType type;
  char *value;
};

Token *new_token(TokenType type, char *value) {
  Token *t = malloc(sizeof(*t));

  t->type = type;
  t->value = value;

  return t;
}

void free_token(Token *t) {
  if (t->type == TK_CONTENT) {
    free(t->value);
  }

  free(t);
}

TokenType token_type(Token *t) {
  return t->type;
}

const char* token_value(Token *t) {
  return t->value;
}

struct tokenizer {
  ReadQueue *rq;
};

Tokenizer *new_tokenizer() {
  Tokenizer *tr = (Tokenizer*)malloc(sizeof(*tr));
  tr->rq = new_read_queue();
  return tr;
}

void free_tokenizer(Tokenizer *tr) {
  free_read_queue(tr->rq);
  free(tr);
}

void tokenizer_write(Tokenizer *tr, char *buf, size_t len) {
  read_queue_write(tr->rq, buf, len);
}

int is_content_char(char ch) {
  return ch != '\x1b' && ch != '\n';
}

Token *read_content_token(Tokenizer *tr) {
  char ch;
  size_t n;
  char *val;
  size_t len;
  FILE *stream = open_memstream(&val, &len);

  while (1) {
    n = read_queue_read(tr->rq, &ch, 1);

    if (n < 1) {
      break;
    }

    if (!is_content_char(ch)) {
      read_queue_rollback(tr->rq);
      break;
    }

    read_queue_commit(tr->rq);

    if (fputc(ch, stream) == EOF) {
      perror("fputc");
      exit(EXIT_FAILURE);
    }
  }

  fclose(stream);

  //fprintf(stderr, "got content token '%s'\n", val);

  return new_token(TK_CONTENT, val);
}

Token *read_termstyle_token(Tokenizer *tr) {
  // TODO: Handle escape sequences
  return new_token(TK_TERMSTYLE, "\x1b");
}

Token *tokenizer_read(Tokenizer *tr) {
  char ch;
  size_t n;

  if ((n = read_queue_read(tr->rq, &ch, 1)) == 0) {
    return NULL;
  }

  if (ch == '\n') {
    read_queue_commit(tr->rq);
    return new_token(TK_NEWLINE, "\n");
  } else if (ch == '\x1b') {
    read_queue_rollback(tr->rq);
    return read_termstyle_token(tr);
  } else if (is_content_char(ch)) {
    read_queue_rollback(tr->rq);
    return read_content_token(tr);
  } else {
    read_queue_commit(tr->rq);
    return new_token(TK_NONE, "");
  }
}
