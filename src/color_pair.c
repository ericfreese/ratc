#include <ncurses.h>

#include "term_style.h"

short num_pairs = 0;

// TODO: Implement as hash map
int color_pair_get(NCURSES_COLOR_T fg, NCURSES_COLOR_T bg) {
  num_pairs++;

  init_pair(num_pairs, fg, bg);

  return num_pairs;
}
