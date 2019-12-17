#pragma once

#include <stdlib.h>
#include <ncurses.h>

#include "esc_seq.h"

typedef struct term_style TermStyle;
struct term_style {
  NCURSES_COLOR_T fg;
  NCURSES_COLOR_T bg;
  int bold;
  int underline;
  int reverse;
};

TermStyle *new_term_style();
TermStyle *term_style_dup(TermStyle *ts);
int term_style_is_default(TermStyle *ts);
attr_t term_style_to_attr(TermStyle *ts);
void term_style_apply(TermStyle *ts, EscSeq *es);
