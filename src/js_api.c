#include <stdio.h>

#include "js_api.h"
#include "key_seq.h"
#include "pager.h"
#include "rat.h"

struct js_event_handler {
  char *stash_key;
  duk_context *ctx;
};

/* Duktape uses a CESU-8 encoding, which allows UTF-16 surrogate pairs
   (themselves encoded in UTF-8), in order to be kinda-sorta compatible with
   ecmascript's UTF-16 requirements. This function just copies the cesu8
   string, converting any surrogate pairs it finds to UTF-8. */
static char *cesu8_to_utf8(const char *cesu8)
{
  char *utf8 = calloc(1, strlen(cesu8) + 1);
  const unsigned char *cc = (void *)cesu8;
  char *cu = utf8;
  uint32_t hs = 0;

  while (*cc != '\0') {
    uint32_t c = 0;
    uint32_t u;

    if (cc[0] <= 0x7F) {
      *cu++ = *cc++;
      continue;
    } else if (cc[0] <= 0xDF) {
      *cu++ = *cc++;
      *cu++ = *cc++;
      continue;
    } else if (cc[0] <= 0xEF) {
      /* Surrogates are encoded in 3 chars so convert
         back to a single UTF-16 value */
      c = ((uint32_t)cc[0] & 0xF) << 12 |
          ((uint32_t)cc[1] & 0x3F) << 6 |
          ((uint32_t)cc[2] & 0x3F);
    } else {
      *cu++ = *cc++;
      *cu++ = *cc++;
      *cu++ = *cc++;
      *cu++ = *cc++;
      continue;
    }
    if (hs == 0 && c >= 0xD800 && c <= 0xDBFF)
      hs = c;
    else if (hs != 0 && c >= 0xDC00 && c <= 0xDFFF) {
      /* Have high and low surrogates - convert to code point then
         back to UTF-8 */
      u = 0x10000 + ((((uint32_t)hs & 0x3FF) << 10) | (c & 0x3FF));
      *cu++ = 0xF0 |  u >> 18;
      *cu++ = 0x80 | (u >> 12 & 0x3F);
      *cu++ = 0x80 | (u >> 6 & 0x3F);
      *cu++ = 0x80 | (u & 0x3F);
      hs = 0;
    } else {
      *cu++ = cc[0];
      *cu++ = cc[1];
      *cu++ = cc[2];
      hs = 0;
    }
    cc += 3;
  }

  *cu = '\0';
  return utf8;
}

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

  /* Put the function from the stash onto the stack and call it */
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

    char *keys = cesu8_to_utf8(duk_get_string(duk_ctx, -1));
    key_seq_add(ks, keys);
    free((char*)keys);

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
  const char *utf8_message = cesu8_to_utf8(duk_get_string(duk_ctx, 0));
  fprintf(stderr, "Rat.print: '%s'\n", utf8_message);
  free((char*)utf8_message);

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

  Pager *p = new_pager(cesu8_to_utf8(duk_get_string(duk_ctx, 0)));
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

duk_ret_t js_pager_add_event_listener(duk_context *duk_ctx) {
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

    char *keys = cesu8_to_utf8(duk_get_string(duk_ctx, -1));
    key_seq_add(ks, keys);
    free((char*)keys);

    duk_pop(duk_ctx);
  }

  // TODO: Fix this js_get_stash_key usage
  JSEventHandler *jeh = new_js_event_handler(duk_ctx, js_get_stash_key(ks));

  /* Save a reference to the handler function in the stash */
  duk_push_heap_stash(duk_ctx);
  duk_dup(duk_ctx, 1);
  duk_put_prop_string(duk_ctx, -2, jeh->stash_key);
  duk_pop(duk_ctx);

  duk_push_this(duk_ctx);
  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_add_event_listener(p, ks, jeh);

  return 0;
}

duk_ret_t js_pager_get_cursor_annotation(duk_context *duk_ctx) {
  if (!duk_is_string(duk_ctx, 0)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "getCursorAnnotation(): first arg must be a string");
  }

  duk_push_this(duk_ctx);
  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  char *annotation_type = cesu8_to_utf8(duk_get_string(duk_ctx, 0));

  Annotation *a = pager_get_cursor_annotation(p, annotation_type);

  free((char*)annotation_type);

  /* TODO: encode in cesu-8 before pushing, see note in duktape docs */
  if (a == NULL) {
    duk_push_null(duk_ctx);
  } else {
    duk_push_string(duk_ctx, annotation_value(a));
  }

  return 1;
}

duk_ret_t js_pager_reload(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_reload(p);

  return 0;
}

duk_ret_t js_pager_move_cursor(duk_context *duk_ctx) {
  if (!duk_is_number(duk_ctx, 0)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "first arg must be a number");
  }

  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_move_cursor(p, duk_get_int(duk_ctx, 0));

  return 0;
}

duk_ret_t js_pager_scroll(duk_context *duk_ctx) {
  if (!duk_is_number(duk_ctx, 0)) {
    return duk_error(duk_ctx, DUK_ERR_TYPE_ERROR, "first arg must be a number");
  }

  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_scroll(p, duk_get_int(duk_ctx, 0));

  return 0;
}

duk_ret_t js_pager_page_up(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_page_up(p);

  return 0;
}

duk_ret_t js_pager_page_down(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_page_down(p);

  return 0;
}

duk_ret_t js_pager_first_line(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_first_line(p);

  return 0;
}

duk_ret_t js_pager_last_line(duk_context *duk_ctx) {
  duk_push_this(duk_ctx);

  duk_get_prop_string(duk_ctx, -1, DUK_HIDDEN_SYMBOL("__pager__"));
  Pager *p = duk_get_pointer(duk_ctx, -1);

  pager_last_line(p);

  return 0;
}

/* Adds a `Pager` constructor to an object on the top of the stack */
void js_pager_setup(duk_context *duk_ctx) {
  duk_push_c_function(duk_ctx, js_new_pager, 1);

  duk_push_object(duk_ctx);

  duk_push_c_function(duk_ctx, js_pager_add_annotator, 1);
  duk_put_prop_string(duk_ctx, -2, "addAnnotator");

  duk_push_c_function(duk_ctx, js_pager_add_event_listener, 2);
  duk_put_prop_string(duk_ctx, -2, "addEventListener");

  duk_push_c_function(duk_ctx, js_pager_get_cursor_annotation, 1);
  duk_put_prop_string(duk_ctx, -2, "getCursorAnnotation");

  duk_push_c_function(duk_ctx, js_pager_reload, 0);
  duk_put_prop_string(duk_ctx, -2, "reload");

  duk_push_c_function(duk_ctx, js_pager_move_cursor, 1);
  duk_put_prop_string(duk_ctx, -2, "moveCursor");

  duk_push_c_function(duk_ctx, js_pager_scroll, 1);
  duk_put_prop_string(duk_ctx, -2, "scroll");

  duk_push_c_function(duk_ctx, js_pager_page_up, 0);
  duk_put_prop_string(duk_ctx, -2, "pageUp");

  duk_push_c_function(duk_ctx, js_pager_page_down, 0);
  duk_put_prop_string(duk_ctx, -2, "pageDown");

  duk_push_c_function(duk_ctx, js_pager_first_line, 0);
  duk_put_prop_string(duk_ctx, -2, "firstLine");

  duk_push_c_function(duk_ctx, js_pager_last_line, 0);
  duk_put_prop_string(duk_ctx, -2, "lastLine");

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

  char *cmd = cesu8_to_utf8(duk_get_string(duk_ctx, 0));
  char *annotation_type = cesu8_to_utf8(duk_get_string(duk_ctx, 1));

  Annotator *ar = new_annotator(cmd, annotation_type);

  free((char*)cmd);
  free((char*)annotation_type);

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
