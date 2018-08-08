#include "minunit.h"
#include "read_queue.h"

#define BUFSIZE 30
ReadQueue *rq;
char buf[BUFSIZE];
size_t n;

MU_TEST(read_returns_0_if_empty) {
  rq = new_read_queue();

  n = read_queue_read(rq, buf, BUFSIZE - 1);
  mu_check(n == 0);

  free_read_queue(rq);
}

MU_TEST(read_copies_data_and_returns_num_bytes) {
  rq = new_read_queue();

  read_queue_write(rq, "foobar", 6);

  n = read_queue_read(rq, buf, BUFSIZE - 1);
  buf[n] = '\0';
  mu_check(n == 6);
  mu_check(strcmp(buf, "foobar") == 0);

  free_read_queue(rq);
}

MU_TEST(read_supports_partial_reads) {
  rq = new_read_queue();

  read_queue_write(rq, "foobar", 6);

  n = read_queue_read(rq, buf, 3);
  buf[n] = '\0';
  mu_check(n == 3);
  mu_check(strcmp(buf, "foo") == 0);

  n = read_queue_read(rq, buf, 3);
  buf[n] = '\0';
  mu_check(n == 3);
  mu_check(strcmp(buf, "bar") == 0);

  n = read_queue_read(rq, buf, BUFSIZE - 1);
  mu_check(n == 0);

  free_read_queue(rq);
}

MU_TEST(supports_interleaved_reads_and_writes) {
  rq = new_read_queue();

  read_queue_write(rq, "foo", 3);

  n = read_queue_read(rq, buf, 2);
  buf[n] = '\0';
  mu_check(n == 2);
  mu_check(strcmp(buf, "fo") == 0);

  read_queue_write(rq, "bar", 3);

  n = read_queue_read(rq, buf, 2);
  buf[n] = '\0';
  mu_check(n == 2);
  mu_check(strcmp(buf, "ob") == 0);

  read_queue_write(rq, "baz", 3);

  n = read_queue_read(rq, buf, BUFSIZE - 1);
  buf[n] = '\0';
  mu_check(n == 5);
  mu_check(strcmp(buf, "arbaz") == 0);

  free_read_queue(rq);
}

MU_TEST(rollback_resets_to_last_commit) {
  rq = new_read_queue();

  read_queue_write(rq, "foobar", 6);

  n = read_queue_read(rq, buf, 3);
  buf[n] = '\0';
  mu_check(n == 3);
  mu_check(strcmp(buf, "foo") == 0);

  read_queue_rollback(rq);

  n = read_queue_read(rq, buf, 3);
  buf[n] = '\0';
  mu_check(n == 3);
  mu_check(strcmp(buf, "foo") == 0);

  read_queue_commit(rq);

  n = read_queue_read(rq, buf, 3);
  buf[n] = '\0';
  mu_check(n == 3);
  mu_check(strcmp(buf, "bar") == 0);

  free_read_queue(rq);
}

int main(int argc, char *argv[]) {
  MU_RUN_TEST(read_returns_0_if_empty);
  MU_RUN_TEST(read_copies_data_and_returns_num_bytes);
  MU_RUN_TEST(read_supports_partial_reads);
  MU_RUN_TEST(supports_interleaved_reads_and_writes);
  MU_RUN_TEST(rollback_resets_to_last_commit);

  MU_REPORT();

  return minunit_status;
}
