#pragma once

#include <ncurses.h>

#include "box.h"

typedef struct render_line RenderLine;
RenderLine *new_render_line();
void free_render_line(RenderLine *rl);
void render_line_add(RenderLine *rl, attr_t attr, const char *content);
void render_line_draw(RenderLine *rl, int y, int x);

typedef struct render_lines RenderLines;
RenderLines *new_render_lines();
void free_render_lines(RenderLines *rls);
void render_lines_add(RenderLines *rls, RenderLine *rl);
void render_lines_draw(RenderLines *rls, Box *box);
