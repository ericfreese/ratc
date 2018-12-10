#pragma once

#include <stdlib.h>
#include <ncurses.h>

#include "term_style.h"

typedef struct highlight Highlight;
TermStyle *highlight_get_style(Highlight *h);

typedef struct highlights Highlights;
Highlights *new_highlights();
void free_highlights(Highlights *hs);
void highlights_end(Highlights *hs, size_t offset);
void highlights_start(Highlights *hs, TermStyle *ts, size_t offset);
Highlight *highlight_at_point(Highlights *hs, size_t point);
