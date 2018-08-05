#pragma once

#include <sys/wait.h>

#include "annotator.h"
#include "buffer.h"

void io_init(void (*tty_handler)(int ch));
void io_cleanup();
void io_start_buffer(Buffer *b, const char *cmd);
void io_start_annotating_buffer(Buffer *b, Annotator *ar);
void io_close_buffer(Buffer *b);
void io_tick();
