#pragma once

#include "duktape.h"

typedef struct js_event_handler JSEventHandler;
void js_run_event_handler(JSEventHandler *jeh);
void js_free_event_handler(JSEventHandler *jeh);

void js_rat_setup(duk_context *duk_ctx);
