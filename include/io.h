#pragma once

#include <sys/wait.h>
#include <vterm.h>

#include "annotator.h"
#include "buffer.h"

// TODO: See `g_io_channel_unix_new()` and `g_io_add_watch()` for some inspiration here

void io_init(void (*tty_handler)(int ch));
void io_cleanup();
void io_start_vterm(VTerm *vt, const char *cmd);
//void io_start_annotating_buffer(Buffer *b, Annotator *ar);
void io_close_buffer(VTerm *vt);
void io_tick();
