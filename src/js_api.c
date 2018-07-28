#include "js_api.h"
#include "pager.h"
#include "rat.h"

duk_ret_t rat_print(duk_context *duk_ctx) {
  const char *message = duk_get_string(duk_ctx, 0);

  fprintf(stderr, "print: '%s'\n", message);

  return 0;
}

duk_ret_t rat_print_pager(duk_context *duk_ctx) {
  duk_bool_t ret = duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__pager__"));

  if (!ret) {
    duk_pop(duk_ctx);
    return 0;
  }

  Pager *p = (Pager*)duk_get_pointer(duk_ctx, -1);

  fprintf(stderr, "printing pager: %p\n", p);

  duk_pop(duk_ctx);

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

  fprintf(stderr, "created pager: %p\n", p);

  duk_push_this(duk_ctx);

  duk_push_c_function(duk_ctx, js_finalize_pager, 1);
  duk_set_finalizer(duk_ctx, -2);

  duk_push_pointer(duk_ctx, p);
  duk_put_prop_string(duk_ctx, -2, DUK_HIDDEN_SYMBOL("__pager__"));

  return 0;
}

/* Adds a `Pager` constructor to an object on the top of the stack */
void js_pager_setup(duk_context *duk_ctx) {
  duk_push_c_function(duk_ctx, js_new_pager, 1);

  duk_push_object(duk_ctx);

  //duk_push_c_function(duk_ctx, js_finalize_pager, 1);
  //duk_set_finalizer(duk_ctx, -2);

  duk_put_prop_string(duk_ctx, -2, "prototype");

  duk_put_prop_string(duk_ctx, -2, "Pager");
}

duk_ret_t js_rat_push(duk_context *duk_ctx) {
  duk_get_prop_string(duk_ctx, 0, DUK_HIDDEN_SYMBOL("__pager__"));
  rat_push_pager(duk_get_pointer(duk_ctx, -1));

  return 0;
}

void js_rat_setup(duk_context *duk_ctx) {
  duk_push_object(duk_ctx);

  duk_push_c_function(duk_ctx, rat_print, 1);
  duk_put_prop_string(duk_ctx, -2, "print");

  duk_push_c_function(duk_ctx, rat_print_pager, 1);
  duk_put_prop_string(duk_ctx, -2, "printPager");

  js_pager_setup(duk_ctx);

  duk_push_c_function(duk_ctx, js_rat_push, 1);
  duk_put_prop_string(duk_ctx, -2, "push");

  duk_put_global_string(duk_ctx, "Rat");
}
