#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "esc_seq.h"
#include "term_style.h"
#include "tokenizer.h"
#include "read_queue.h"

typedef enum {
  TR_DEFAULT = 0,
  TR_ESC,
  TR_CS_PARAM,
  TR_CS_INTER
} TokenizerState;

struct tokenizer {
  ReadQueue *rq;
  TermStyle *ts;
};

Tokenizer *new_tokenizer() {
  Tokenizer *tr = malloc(sizeof *tr);
  tr->rq = new_read_queue();
  tr->ts = new_term_style();
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

Token *read_newline_token(Tokenizer *tr) {
  read_queue_advance(tr->rq, 1);
  return new_newline_token("\n");
}

Token *read_content_token(Tokenizer *tr) {
  char ch;
  size_t n;
  char *val;
  size_t len;
  FILE *stream = open_memstream(&val, &len);
  Token *t;

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

  t = new_content_token(val);
  free(val);
  return t;
}

EscSeqPart *read_esc_seq_cs_priv_param(Tokenizer *tr) {
  EscSeqPart *esp;

  char ch;
  char *val;
  size_t len;
  FILE *stream = open_memstream(&val, &len);

  while (1) {
    ch = read_queue_peek(tr->rq);

    if (!(ch = read_queue_peek(tr->rq))) {
      break;
    }

    if (ch >= 0x30 && ch <= 0x3f) {
      if (fputc(ch, stream) == EOF) {
        perror("fputc");
        exit(EXIT_FAILURE);
      }
    } else {
      fclose(stream);
      free(val);
      return NULL;
    }
  }

  fclose(stream);
  esp = new_esc_seq_part(ESP_PRIV_PARAM, val);
  free(val);

  return esp;
}

EscSeqPart *read_esc_seq_cs_param_num(Tokenizer *tr) {
  EscSeqPart *esp;

  char ch;
  char *val;
  size_t len;
  FILE *stream = open_memstream(&val, &len);

  while (1) {
    ch = read_queue_peek(tr->rq);

    if (ch >= 0x30 && ch <= 0x39) {
      if (fputc(ch, stream) == EOF) {
        perror("fputc");
        exit(EXIT_FAILURE);
      }

      read_queue_advance(tr->rq, 1);
    } else {
      break;
    }
  }

  fclose(stream);
  esp = new_esc_seq_part(ESP_PARAM_NUM, val);
  free(val);

  return esp;
}

EscSeq *read_esc_seq(Tokenizer *tr) {
  if (read_queue_peek(tr->rq) != 0x1b) {
    return NULL;
  }

  EscSeq *es = new_esc_seq();
  EscSeqPart *esp;

  TokenizerState state;

  char buf[2];
  buf[1] = '\0';

  read_queue_read(tr->rq, &buf, 1);
  state = TR_ESC;
  esc_seq_add_part(es, new_esc_seq_part(ESP_ESC, buf));

  while (state != TR_DEFAULT) {
    if (!(*buf = read_queue_peek(tr->rq))) {
      goto abort;
    }

    switch (state) {
      case TR_ESC:
        if (*buf >= 0x40 && *buf <= 0x5f) {
          if (*buf == '[') {
            state = TR_CS_PARAM;
          } else {
            state = TR_DEFAULT;
          }

          esc_seq_add_part(es, new_esc_seq_part(ESP_FE, buf));
        } else {
          state = TR_DEFAULT;
          esc_seq_add_part(es, new_esc_seq_part(ESP_UNKNOWN, buf));
        }

        read_queue_advance(tr->rq, 1);
        break;

      case TR_CS_PARAM:
        if (*buf >= 0x20 && *buf <= 0x2f) {
          state = TR_CS_INTER;
          esc_seq_add_part(es, new_esc_seq_part(ESP_INTER, buf));
          read_queue_advance(tr->rq, 1);
        } else if (*buf >= 0x30 && *buf <= 0x39) {
          if ((esp = read_esc_seq_cs_param_num(tr)) == NULL) {
            goto abort;
          }

          esc_seq_add_part(es, esp);
        } else if (*buf == 0x3a) {
          esc_seq_add_part(es, new_esc_seq_part(ESP_PARAM_SEP, buf));
          read_queue_advance(tr->rq, 1);
        } else if (*buf == 0x3b) {
          esc_seq_add_part(es, new_esc_seq_part(ESP_SEP, buf));
          read_queue_advance(tr->rq, 1);
        } else if (es->last->type == ESP_FE && *buf >= 0x3c && *buf <= 0x3f) {
          if ((esp = read_esc_seq_cs_priv_param(tr)) == NULL) {
            goto abort;
          }

          esc_seq_add_part(es, esp);
        } else if (*buf >= 0x40 && *buf <= 0x7e) {
          state = TR_DEFAULT;
          esc_seq_add_part(es, new_esc_seq_part(ESP_FINAL, buf));
          read_queue_advance(tr->rq, 1);
        } else {
          state = TR_DEFAULT;
          esc_seq_add_part(es, new_esc_seq_part(ESP_UNKNOWN, buf));
          read_queue_advance(tr->rq, 1);
        }
        break;

      case TR_CS_INTER:
        if (*buf >= 0x20 && *buf <= 0x2f) {
          esc_seq_add_part(es, new_esc_seq_part(ESP_INTER, buf));
        } else if (*buf >= 0x40 && *buf <= 0x7e) {
          state = TR_DEFAULT;
          esc_seq_add_part(es, new_esc_seq_part(ESP_FINAL, buf));
        } else {
          state = TR_DEFAULT;
          esc_seq_add_part(es, new_esc_seq_part(ESP_UNKNOWN, buf));
        }

        read_queue_advance(tr->rq, 1);
        break;
      case TR_DEFAULT:
        /* not possible */
        goto abort;
    }
  }

  read_queue_commit(tr->rq);
  return es;

abort:
  free_esc_seq(es);
  read_queue_rollback(tr->rq);
  return NULL;
}

Token *read_escape_sequence_token(Tokenizer *tr) {
  EscSeq *es = read_esc_seq(tr);

  if (es == NULL) {
    return NULL;
  }

  term_style_apply(tr->ts, es);

  free_esc_seq(es);

  return new_termstyle_token(tr->ts);
}

Token *tokenizer_read(Tokenizer *tr) {
  char ch;

  if (!read_queue_readable(tr->rq)) {
    return NULL;
  }

  ch = read_queue_peek(tr->rq);

  if (ch == '\n') {
    return read_newline_token(tr);
  } else if (ch == '\x1b') {
    return read_escape_sequence_token(tr);
  } else {
    return read_content_token(tr);
  }
}
