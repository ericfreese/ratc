#include "minunit.h"
#include "tokenizer.h"

Tokenizer *tr;
Token *t;

MU_TEST(read_returns_null_if_no_content) {
  tr = new_tokenizer();

  t = tokenizer_read(tr);
  mu_check(t == NULL);

  free_tokenizer(tr);
}

MU_TEST(read_returns_content_token) {
  tr = new_tokenizer();

  tokenizer_write(tr, "foo", 3);
  tokenizer_write(tr, "bar", 3);
  tokenizer_write(tr, "baz", 3);

  t = tokenizer_read(tr);

  mu_check(token_type(t) == TK_CONTENT);
  mu_check(strcmp(token_value(t), "foobarbaz") == 0);

  free_token(t);

  free_tokenizer(tr);
}

MU_TEST(read_returns_null_until_it_has_a_full_token) {
  tr = new_tokenizer();

  tokenizer_write(tr, "\e[31", 3);

  t = tokenizer_read(tr);
  mu_check(t == NULL);

  tokenizer_write(tr, "1mfoo\e", 6);

  t = tokenizer_read(tr);
  mu_check(token_type(t) == TK_TERMSTYLE);
  //mu_check(... red ...);
  free_token(t);

  t = tokenizer_read(tr);
  mu_check(token_type(t) == TK_CONTENT);
  mu_check(strcmp(token_value(t), "foo") == 0);
  free_token(t);

  t = tokenizer_read(tr);
  mu_check(t == NULL);

  tokenizer_write(tr, "0m", 2);

  t = tokenizer_read(tr);
  mu_check(token_type(t) == TK_TERMSTYLE);
  //mu_check(... reset ...);
  free_token(t);

  free_tokenizer(tr);
}

int main(int argc, char *argv[]) {
  MU_RUN_TEST(read_returns_null_if_no_content);
  MU_RUN_TEST(read_returns_content_token);
  MU_RUN_TEST(read_returns_null_until_it_has_a_full_token);

  MU_REPORT();

  return minunit_status;
}
