#pragma once

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "annotation.h"
#include "render_lines.h"
#include "token.h"

typedef struct annotations Annotations;
Annotations *new_annotations();
void free_annotations(Annotations *as);
void annotations_add(Annotations *as, Annotation *a);
Annotations *annotations_intersecting(Annotations *as, uint32_t start, uint32_t end);

typedef struct buffer Buffer;
Buffer *new_buffer();
void free_buffer(Buffer *b);
void buffer_handle_token(Buffer *b, Token *t);
RenderLines *get_render_lines(Buffer *b, size_t start, size_t num);
//void buffer_list_annotations(Buffer *b);
void buffer_add_annotation(Buffer *b, Annotation *a);
size_t buffer_len(Buffer *b);
size_t buffer_num_lines(Buffer *b);
const char* buffer_content(Buffer *b, size_t offset);
int buffer_is_running(Buffer *b);
