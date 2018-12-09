#include <string.h>
#include <stdlib.h>

#include "minunit.h"
#include "buffer.h"

MU_TEST(test_new) {
  Buffer *b = new_buffer();

  buffer_handle_token(b, new_content_token("foo bar"));

  mu_check(buffer_len(b) == 7);
  mu_check(strcmp(buffer_content(b, 0), "foo bar") == 0);
}

MU_TEST_SUITE(test_suite) {
  MU_RUN_TEST(test_new);
}

int main(int argc, char *argv[]) {
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return minunit_status;
}

