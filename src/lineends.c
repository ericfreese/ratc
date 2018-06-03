#include "rat.h"

LineEnds *new_line_ends() {
  LineEnds *le = (LineEnds*)malloc(sizeof(LineEnds));

  le->len = 0;
  le->size = 8;
  le->offsets = (size_t*)malloc(le->size * sizeof(size_t));

  return le;
}

void free_line_ends(LineEnds *le) {
  free(le);
}

void push_line_end(LineEnds *le, size_t offset) {
  if (le->len == le->size) {
    le->size *= 2;
    le->offsets = (size_t*)realloc(le->offsets, le->size);
  }

  le->offsets[le->len] = offset;

  le->len++;
}

