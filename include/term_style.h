#pragma once

#include <stdlib.h>
#include <ncurses.h>

#include "esc_seq.h"

typedef uint16_t tsattr_t;

typedef struct term_style TermStyle;
struct term_style {
  tsattr_t fg;
  tsattr_t bg;
};

TermStyle *new_term_style();
void term_style_apply(TermStyle *ts, EscSeq *es);
