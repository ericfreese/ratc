#include <string.h>

#include "color_pair.h"
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

  ts->fg = ts->bg = -1;
  ts->bold = ts->underline = ts->reverse = 0;

  return ts;
}

TermStyle *term_style_dup(TermStyle *ts) {
  TermStyle *dup = malloc(sizeof *ts);

  dup->fg = ts->fg;
  dup->bg = ts->bg;
  dup->bold = ts->bold;
  dup->underline = ts->underline;
  dup->reverse = ts->reverse;

  return dup;
}

int term_style_is_default(TermStyle *ts) {
  return ts->fg == -1 && ts->bg == -1 && !ts->bold && !ts->underline && !ts->reverse;
}

attr_t term_style_to_attr(TermStyle *ts) {
  attr_t a = COLOR_PAIR(color_pair_get(ts->fg, ts->bg));

  if (ts->bold) {
    a |= A_BOLD;
  }

  if (ts->underline) {
    a |= A_UNDERLINE;
  }

  if (ts->reverse) {
    a |= A_REVERSE;
  }

  return a;
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

  NCURSES_COLOR_T new_fg = ts->fg;
  NCURSES_COLOR_T new_bg = ts->bg;

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
    new_fg = new_bg = -1;
    ts->bold = ts->underline = ts->reverse = 0;
  }

  for (SgrCode *sc = sgr->first; sc != NULL; sc = sc->next) {
    if (sc->code == 0) {
      new_fg = new_bg = -1;
      ts->bold = ts->underline = ts->reverse = 0;
    } else if (sc->code == 1) {
      ts->bold = 1;
    } else if (sc->code == 4) {
      ts->underline = 1;
    } else if (sc->code == 7) {
      ts->reverse = 1;
    } else if (sc->code == 22) {
      ts->bold = 0;
    } else if (sc->code == 24) {
      ts->underline = 0;
    } else if (sc->code == 27) {
      ts->reverse = 0;
    } else if (sc->code >= 30 && sc->code <= 37) {
      new_fg = sc->code - 30;
    } else if (sc->code == 38) {
      sc = sc->next;
      if (sc != NULL && sc->code == 5) {
        sc = sc->next;
        if (sc != NULL) {
          new_fg = sc->code;
        }
      }
    } else if (sc->code == 39) {
      new_fg = -1;
    } else if (sc->code >= 40 && sc->code <= 47) {
      new_bg = sc->code - 40;
    } else if (sc->code == 48) {
      sc = sc->next;
      if (sc != NULL && sc->code == 5) {
        sc = sc->next;
        if (sc != NULL) {
          new_bg = sc->code;
        }
      }
    } else if (sc->code == 49) {
      new_bg = -1;
    }
  }

  ts->fg = new_fg;
  ts->bg = new_bg;

cleanup:
  free_sgr_codes(sgr);
}
