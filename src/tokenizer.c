#include "rat.h"

Tokenizer *new_tokenizer(int fd) {
  Tokenizer *tr = (Tokenizer*)malloc(sizeof(*tr));
  tr->fd = fd;
  return tr;
}

void free_tokenizer(Tokenizer *tr) {
  free(tr);
}

ssize_t read_token(Tokenizer *tr, Token *t) {
  char buf[2];

  ssize_t n = read(tr->fd, buf, 1);

  if (n > 0) {
    buf[n] = '\0';
    strbuf_write(t->value, buf);

    if (buf[0] == '\n') {
      t->type = TK_NEWLINE;
    } else {
      t->type = TK_TEXT;
    }
  } else {
    t->type = TK_NONE;
  }

  return n;
}
