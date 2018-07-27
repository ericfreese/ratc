#include "js_api.h"
#include "pager.h"

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

duk_ret_t js_new_pager(duk_context *duk_ctx) {
  if (!duk_is_constructor_call(duk_ctx)) {
    // TODO: Throw error here?
    return 0;
  }

  Pager *p = new_pager((char*)duk_get_string(duk_ctx, 0));

  fprintf(stderr, "created pager: %p\n", p);

  duk_push_this(duk_ctx);

  duk_push_pointer(duk_ctx, p);

  duk_put_prop_string(duk_ctx, -2, DUK_HIDDEN_SYMBOL("__pager__"));

  return 1;
}

void js_rat_setup(duk_context *duk_ctx) {
  duk_push_object(duk_ctx);

  // [ {} ]




  duk_push_c_function(duk_ctx, rat_print_pager, 1);
  duk_put_prop_string(duk_ctx, -2, "printPager");




  duk_push_c_function(duk_ctx, rat_print, 1);

  // [ {}, fn() ]

  duk_put_prop_string(duk_ctx, -2, "print");

  // [ { print: fn() } ]


  duk_push_c_function(duk_ctx, js_new_pager, 1);

  // [ { ... }, fn() ]

  duk_put_prop_string(duk_ctx, -2, "Pager");

  //Rat.print
  //Rat.Pager


  // Finally add Rat to the global object

  duk_put_global_string(duk_ctx, "Rat");

  // [ ]
}


//void js_pager_setup(duk_context *duk_ctx, duk_idx_t rat_idx) {
//
//}
