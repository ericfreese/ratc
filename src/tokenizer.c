#include "rat.h"

Tokenizer *new_tokenizer(int fd) {
  Tokenizer *tr = (Tokenizer*)malloc(sizeof(*tr));
  tr->fd = fd;
  return tr;
}

void free_tokenizer(Tokenizer *tr) {
  free(tr);
}

int unread_byte(Tokenizer *tr, char *byte) {
  if (tr->was_unread) {
    return 1;
  }

  tr->unread_byte = *byte;
  return 0;
}

ssize_t read_byte(Tokenizer *tr, char* byte) {
  if (tr->was_unread) {
    *byte = tr->unread_byte;
    tr->was_unread = 0;
  } else {
    return read(tr->fd, byte, 1);
  }
}

ssize_t read_token(Tokenizer *tr, Token *t) {
  char buf[2];

  ssize_t n = read_byte(tr, buf);

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
