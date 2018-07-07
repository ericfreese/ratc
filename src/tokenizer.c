#include "tokenizer.h"

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

Tokenizer *new_tokenizer() {
  Tokenizer *tr = (Tokenizer*)malloc(sizeof(*tr));
  tr->read_buffer = open_memstream(&tr->read_buffer_str, &tr->read_buffer_len);
  return tr;
}

void free_tokenizer(Tokenizer *tr) {
  fclose(tr->read_buffer);
  free(tr->read_buffer_str);
  free(tr);
}

void tokenizer_buffer_input(Tokenizer *tr, char *str, size_t len) {
  ssize_t n = fwrite(str, sizeof(*str), len, tr->read_buffer);

  if (n < len) {
    perror("fwrite");
    exit(EXIT_FAILURE);
  }

  fflush(tr->read_buffer);
}

int is_content_char(char ch) {
  return ch != '\x1b' && ch != '\n';
}

Token *read_content_token(Tokenizer *tr, int first) {
  int ch;
  char *val;
  size_t len;
  FILE *stream = open_memstream(&val, &len);

  fputc(first, stream);

  while (1) {
    ch = fgetc(tr->read_buffer);

    if (ch == EOF) {
      break;
    }

    if (!is_content_char((char)ch)) {
      if (ungetc(ch, tr->read_buffer) == EOF) {
        perror("ungetc");
        exit(EXIT_FAILURE);
      }

      break;
    }

    if (fputc(ch, stream) == EOF) {
      perror("fputc");
      exit(EXIT_FAILURE);
    }
  }

  fclose(stream);

  return new_token(TK_CONTENT, val);
}

Token *read_token(Tokenizer *tr) {
  int ch;

  if ((ch = fgetc(tr->read_buffer)) == EOF) {
    return NULL;
  }

  if (ch == '\n') {
    return new_token(TK_NEWLINE, "\n");
  } else if (ch == '\x1b') {
    // TODO: Handle escape sequences
    return new_token(TK_TERMSTYLE, "\x1b");
  } else if (is_content_char(ch)) {
    return read_content_token(tr, ch);
  } else {
    return new_token(TK_NONE, "");
  }
}
