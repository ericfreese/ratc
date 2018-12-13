#include <ncurses.h>

#include "term_style.h"

int pairs[64];

// TODO: Implement as hash map
int color_pair_get(NCURSES_COLOR_T fg, NCURSES_COLOR_T bg) {
  short pair = 8 * (fg % 8) + (bg % 8);

  if (!pairs[pair]) {
    init_pair(pair, fg, bg);
    pairs[pair] = 1;
  }

  return pair;
}
