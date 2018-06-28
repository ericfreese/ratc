#ifndef LINE_ENDS_H
#define LINE_ENDS_H

#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t *offsets;
  size_t len;
  size_t size;
} LineEnds;

LineEnds *new_line_ends();
void free_line_ends(LineEnds *le);
void push_line_end(LineEnds *le, size_t offset);

#endif
