#include <string.h>

#include "esc_seq.h"
#include "term_style.h"

typedef struct sgr_code SgrCode;
struct sgr_code {
  uint16_t code;
  SgrCode *next;
};

typedef struct sgr_codes SgrCodes;
struct sgr_codes {
  size_t len;
  SgrCode *first;
  SgrCode *last;
};

SgrCodes *new_sgr_codes() {
  SgrCodes *sgr = malloc(sizeof *sgr);

  sgr->len = 0;
  sgr->first = NULL;
  sgr->last = NULL;

  return sgr;
}

void free_sgr_codes(SgrCodes *sgr) {
  SgrCode *next;

  for (SgrCode *cursor = sgr->first; cursor != NULL; cursor = next) {
    next = cursor->next;
    free(cursor);
  }

  free(sgr);
}

void sgr_codes_add(SgrCodes *sgr, uint16_t code) {
  SgrCode *sc = malloc(sizeof *sc);

  sc->code = code;
  sc->next = NULL;

  if (sgr->first == NULL) {
    sgr->first = sgr->last = sc;
  } else {
    sgr->last->next = sc;
    sgr->last = sc;
  }

  sgr->len++;
}

TermStyle *new_term_style() {
  TermStyle *ts = malloc(sizeof *ts);

  ts->fg = 0;
  ts->bg = 0;

  return ts;
}

void term_style_apply(TermStyle *ts, EscSeq *es) {
  EscSeqPart *cursor = es->first;
  if (cursor->type != ESP_ESC) {
    return;
  }

  cursor = cursor->next;
  if (cursor->type != ESP_FE || *cursor->value != '[') {
    return;
  }

  tsattr_t new_fg = ts->fg;
  tsattr_t new_bg = ts->bg;

  SgrCodes *sgr = new_sgr_codes();

  for (EscSeqPart *cursor = es->first; cursor != NULL; cursor = cursor->next) {
    if (cursor->type == ESP_INTER) {
      goto cleanup;
    }

    if (cursor->type == ESP_FINAL && *cursor->value != 'm') {
      goto cleanup;
    }

    if (cursor->type == ESP_PARAM_NUM) {
      sgr_codes_add(sgr, atoi(cursor->value));
    }
  }

  if (sgr->len == 0) {
    new_fg = COLOR_WHITE;
    new_bg = COLOR_BLACK;
  }

  for (SgrCode *sc = sgr->first; sc != NULL; sc = sc->next) {
    // TODO: Support the rest
    if (sc->code == 0) {
      new_fg = COLOR_WHITE;
      new_bg = COLOR_BLACK;
    } else if (sc->code >= 30 && sc->code <= 37) {
      new_fg = sc->code - 30;
    } else if (sc->code >= 40 && sc->code <= 47) {
      new_bg = sc->code - 40;
    }
  }

  ts->fg = new_fg;
  ts->bg = new_bg;

cleanup:
  free_sgr_codes(sgr);
}
