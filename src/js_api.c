#include <stdio.h>

#include "js_api.h"
#include "key_seq.h"
#include "pager.h"
#include "rat.h"

struct js_event_handler {
  char *stash_key;
  duk_context *ctx;
};

char *js_get_stash_key(void *p) {
  int len = snprintf(NULL, 0, "%p", p);
  char *key = malloc(len + 1);
  sprintf(key, "%p", p);

  return key;
}

JSEventHandler *new_js_event_handler(duk_context *duk_ctx, char *stash_key) {
  JSEventHandler *jeh = malloc(sizeof *jeh);

  jeh->stash_key = stash_key;
  jeh->ctx = duk_ctx;

  return jeh;
}

void js_run_event_handler(JSEventHandler *jeh) {
  fprintf(stderr, "Running handler: '%s'\n", jeh->stash_key);
  duk_push_heap_stash(jeh->ctx);
  duk_get_prop_string(jeh->ctx, -1, jeh->stash_key);
  duk_call(jeh->ctx, 0);
  duk_pop(jeh->ctx);

  /* Pop the return value too */
  duk_pop(jeh->ctx);
}

void js_free_event_handler(JSEventHandler *jeh) {
  duk_push_heap_stash(jeh->ctx);
  duk_del_prop_string(jeh->ctx, -1, jeh->stash_key);
  duk_pop(jeh->ctx);

  free(jeh->stash_key);
  free(jeh);
}

duk_ret_t js_add_event_listener(duk_context *duk_ctx) {
  if (!duk_is_array(duk_ctx, 0)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "first arg must be an array");
  }

  duk_size_t len = duk_get_length(duk_ctx, 0);
  if (len <= 0) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "array must not be empty");
  }

  if (!duk_is_function(duk_ctx, 1)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "second arg must be a function");
  }

  KeySeq *ks = new_key_seq();

  for (duk_size_t i = 0; i < len; i++) {
    duk_get_prop_index(duk_ctx, 0, i);

    if (!duk_is_string(duk_ctx, -1)) {
      free_key_seq(ks);
      return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "array must contain only strings");
    }

    key_seq_add(ks, (char*)duk_get_string(duk_ctx, -1));

    duk_pop(duk_ctx);
  }

  JSEventHandler *jeh = new_js_event_handler(duk_ctx, js_get_stash_key(ks));

  /* Save a reference to the handler function in the stash */
  duk_push_heap_stash(duk_ctx);
  duk_dup(duk_ctx, 1);
  duk_put_prop_string(duk_ctx, -2, jeh->stash_key);
  duk_pop(duk_ctx);

  rat_add_event_listener(ks, jeh);

  return 0;
}

duk_ret_t rat_print(duk_context *duk_ctx) {
  const char *message = duk_get_string(duk_ctx, 0);

  fprintf(stderr, "print: '%s'\n", message);

  return 0;
}

duk_ret_t js_finalize_pager(duk_context *duk_ctx) {
  duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__pager__"));

  fprintf(stderr, "finalizing pager: %p\n", duk_get_pointer(duk_ctx, -1));
  pager_ref_dec(duk_get_pointer(duk_ctx, -1));

  return 0;
}

duk_ret_t js_new_pager(duk_context *duk_ctx) {
  if (!duk_is_constructor_call(duk_ctx)) {
    return 0;
  }

  Pager *p = new_pager((char*)duk_get_string(duk_ctx, 0));
  char *stash_key = js_get_stash_key(p);

  fprintf(stderr, "created pager: %p\n", p);

  duk_push_this(duk_ctx);

  duk_push_c_function(duk_ctx, js_finalize_pager, 1);
  duk_set_finalizer(duk_ctx, -2);

  duk_push_pointer(duk_ctx, p);
  duk_put_prop_string(duk_ctx, -2, DUK_HIDDEN_SYMBOL("__pager__"));

  /* Save a reference to the pager in the stash */
  duk_push_heap_stash(duk_ctx);
  duk_dup(duk_ctx, -2);
  duk_put_prop_string(duk_ctx, -2, stash_key); // stash->{key} = this
  duk_pop(duk_ctx);

  free(stash_key);
  return 0;
}

duk_ret_t js_pager_add_annotator(duk_context *duk_ctx) {
  duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__annotator__"));
  Annotator *ar = duk_get_pointer(duk_ctx, -1);

  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_add_annotator(p, ar);

  return 0;
}

duk_ret_t js_pager_reload(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_reload(p);

  return 0;
}

duk_ret_t js_pager_scroll(duk_context *duk_ctx) {
  if (!duk_is_number(duk_ctx, 0)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "first arg must be an number");
  }

  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_scroll(p, duk_get_int(duk_ctx, 0));

  return 0;
}

/* Adds a `Pager` constructor to an object on the top of the stack */
void js_pager_setup(duk_context *duk_ctx) {
  duk_push_c_function(duk_ctx, js_new_pager, 1);

  duk_push_object(duk_ctx);

  duk_push_c_function(duk_ctx, js_pager_add_annotator, 1);
  duk_put_prop_string(duk_ctx, -2, "addAnnotator");

  duk_push_c_function(duk_ctx, js_pager_reload, 1);
  duk_put_prop_string(duk_ctx, -2, "reload");

  duk_push_c_function(duk_ctx, js_pager_scroll, 1);
  duk_put_prop_string(duk_ctx, -2, "scroll");

  duk_put_prop_string(duk_ctx, -2, "prototype");

  duk_put_prop_string(duk_ctx, -2, "Pager");
}

duk_ret_t js_finalize_annotator(duk_context *duk_ctx) {
  duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__annotator__"));

  fprintf(stderr, "finalizing annotator: %p\n", duk_get_pointer(duk_ctx, -1));
  annotator_ref_dec(duk_get_pointer(duk_ctx, -1));

  return 0;
}

duk_ret_t js_new_annotator(duk_context *duk_ctx) {
  if (!duk_is_constructor_call(duk_ctx)) {
    return 0;
  }

  Annotator *ar = new_annotator((char*)duk_get_string(duk_ctx, 0), (char*)duk_get_string(duk_ctx, 1));

  fprintf(stderr, "created annotator: %p\n", ar);

  duk_push_this(duk_ctx);

  duk_push_c_function(duk_ctx, js_finalize_annotator, 1);
  duk_set_finalizer(duk_ctx, -2);

  duk_push_pointer(duk_ctx, ar);
  duk_put_prop_string(duk_ctx, -2, DUK_HIDDEN_SYMBOL("__annotator__"));

  return 0;
}

/* Adds a `Annotator` constructor to an object on the top of the stack */
void js_annotator_setup(duk_context *duk_ctx) {
  duk_push_c_function(duk_ctx, js_new_annotator, 2);

  duk_push_object(duk_ctx);

  //duk_push_c_function(duk_ctx, js_pager_add_annotator, 1);
  //duk_put_prop_string(duk_ctx, -2, "addAnnotator");

  duk_put_prop_string(duk_ctx, -2, "prototype");

  duk_put_prop_string(duk_ctx, -2, "Annotator");
}

duk_ret_t js_rat_push(duk_context *duk_ctx) {
  duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__pager__"));
  rat_push_pager(duk_get_pointer(duk_ctx, -1));

  return 0;
}

duk_ret_t js_rat_pop(duk_context *duk_ctx) {
  Pager *p = rat_active_pager();
  char *stash_key = js_get_stash_key(p);

  duk_push_heap_stash(duk_ctx);
  duk_del_prop_string(duk_ctx, -1, stash_key);
  duk_pop(duk_ctx);

  rat_pop_pager();

  free(stash_key);
  return 0;
}

duk_ret_t js_get_active_pager(duk_context *duk_ctx) {
  Pager *active = rat_active_pager();

  char *stash_key = js_get_stash_key(active);

  duk_push_heap_stash(duk_ctx);
  duk_get_prop_string(duk_ctx, -1, stash_key);

  free(stash_key);
  return 1;
}

void js_rat_setup(duk_context *duk_ctx) {
  duk_push_object(duk_ctx);

  duk_push_c_function(duk_ctx, js_add_event_listener, 2);
  duk_put_prop_string(duk_ctx, -2, "addEventListener");

  duk_push_c_function(duk_ctx, rat_print, 1);
  duk_put_prop_string(duk_ctx, -2, "print");

  js_pager_setup(duk_ctx);
  js_annotator_setup(duk_ctx);

  duk_push_c_function(duk_ctx, js_rat_pop, 0);
  duk_put_prop_string(duk_ctx, -2, "pop");

  duk_push_c_function(duk_ctx, js_rat_push, 1);
  duk_put_prop_string(duk_ctx, -2, "push");

  duk_push_c_function(duk_ctx, js_get_active_pager, 0);
  duk_put_prop_string(duk_ctx, -2, "getActivePager");

  duk_put_global_string(duk_ctx, "Rat");
}
