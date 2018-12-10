#include <ncurses.h>

#include "minunit.h"
#include "highlights.h"
#include "term_style.h"

Highlights *hs;
Highlight *h;
TermStyle *ts;

MU_TEST(empty_highlights) {
  hs = new_highlights();

  h = highlight_at_point(hs, 5);
  mu_check(h == NULL);

  free(hs);
}

MU_TEST(unterminated_highlight) {
  hs = new_highlights();
  ts = new_term_style();

  ts->fg = COLOR_RED;
  highlights_start(hs, ts, 0);

  h = highlight_at_point(hs, 5);
  mu_check(h != NULL);
  mu_check(highlight_get_style(h)->fg == COLOR_RED);
  mu_check(highlight_get_style(h)->bg == -1);

  free(ts);
  free_highlights(hs);
}

MU_TEST(terminated_highlight) {
  hs = new_highlights();
  ts = new_term_style();

  ts->fg = COLOR_RED;
  highlights_start(hs, ts, 0);
  highlights_end(hs, 10);

  h = highlight_at_point(hs, 0);
  mu_check(h != NULL);
  mu_check(highlight_get_style(h)->fg == COLOR_RED);
  mu_check(highlight_get_style(h)->bg == -1);

  h = highlight_at_point(hs, 9);
  mu_check(h != NULL);
  mu_check(highlight_get_style(h)->fg == COLOR_RED);
  mu_check(highlight_get_style(h)->bg == -1);

  h = highlight_at_point(hs, 10);
  mu_check(h == NULL);

  free(ts);
  free_highlights(hs);
}

MU_TEST(multiple_highlights) {
  hs = new_highlights();
  ts = new_term_style();

  ts->fg = COLOR_RED;
  highlights_start(hs, ts, 0);
  highlights_end(hs, 10);

  ts->fg = COLOR_CYAN;
  ts->bg = COLOR_GREEN;
  highlights_start(hs, ts, 10);
  highlights_end(hs, 20);

  h = highlight_at_point(hs, 9);
  mu_check(h != NULL);
  mu_check(highlight_get_style(h)->fg == COLOR_RED);
  mu_check(highlight_get_style(h)->bg == -1);

  h = highlight_at_point(hs, 10);
  mu_check(h != NULL);
  mu_check(highlight_get_style(h)->fg == COLOR_CYAN);
  mu_check(highlight_get_style(h)->bg == COLOR_GREEN);

  free(ts);
  free_highlights(hs);
}

int main(int argc, char *argv[]) {
  MU_RUN_TEST(empty_highlights);
  MU_RUN_TEST(unterminated_highlight);
  MU_RUN_TEST(terminated_highlight);
  MU_RUN_TEST(multiple_highlights);

  MU_REPORT();

  return minunit_status;
}

