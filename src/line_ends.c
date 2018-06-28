#include "line_ends.h"

LineEnds *new_line_ends() {
  LineEnds *le = (LineEnds*)malloc(sizeof(*le));

  le->len = 0;
  le->size = 8;
  le->offsets = (size_t*)malloc(le->size * sizeof(*le->offsets));

  return le;
}

void free_line_ends(LineEnds *le) {
  free(le);
}

void push_line_end(LineEnds *le, size_t offset) {
  if (le->len == le->size) {
    le->size *= 2;
    le->offsets = (size_t*)realloc(le->offsets, le->size * sizeof(*le->offsets));
  }

  le->offsets[le->len] = offset;

  le->len++;
}

