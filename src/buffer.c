#include "rat.h"

Buffer *new_buffer(int fd) {
  Buffer *b = (Buffer*)malloc(sizeof(Buffer));

  b->fd = fd;
  b->stream = new_stream();
  b->line_ends = new_line_ends();

  pthread_create(&b->fill_thread, NULL, fill_buffer, b);

  return b;
}

void free_buffer(Buffer *b) {
  free_stream(b->stream);
  free_line_ends(b->line_ends);
  free(b);
}

void *fill_buffer(void *bptr) {
  Buffer *b = (Buffer*)bptr;
  char buf[1025];
  ssize_t n;

  printw("filling buffer...\n");
  refresh();

  while ((n = read(b->fd, buf, 1024)) > 0) {
    buf[n] = '\0';

    stream_write(b->stream, buf);

    printw("stream: '%s'\n", b->stream->strbuf->str);
    refresh();

    sleep(1);
  }

  printw("done filling buffer\n");
  refresh();

  if (n == -1) {
    perror("fill_buffer");
    exit(EXIT_FAILURE);
  }
}

