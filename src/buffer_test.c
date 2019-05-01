#include <string.h>
#include <stdlib.h>

#include "minunit.h"
#include "buffer.h"

Buffer *b;
TermStyle *ts;
RenderLines *rls;
RenderLine *rl;

MU_TEST(num_lines_returns_number_of_lines) {
  b = new_buffer();

  buffer_handle_token(b, new_content_token("one"));
  buffer_handle_token(b, new_newline_token("\n"));
  buffer_handle_token(b, new_content_token("two"));
  buffer_handle_token(b, new_newline_token("\n"));
  buffer_handle_token(b, new_content_token("three"));
  buffer_handle_token(b, new_newline_token("\n"));

  mu_check(buffer_num_lines(b) == 4);

  free_buffer(b);
}

int main(int argc, char *argv[]) {
  MU_RUN_TEST(num_lines_returns_number_of_lines);

  MU_REPORT();

  return minunit_status;
}

