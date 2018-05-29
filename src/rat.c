#include "rat.h"

struct flags {
  char *cmd;
  char *modes;
} initial_flags;

char* join_strings(char **strings) {
  int size = 1;

  char *result = (char*)malloc(size);
  char **string = strings;

  int len = 0;
  int extra;

  result[0] = '\0';

  while (*string != NULL) {
    extra = strlen(*string);

    while (len + 1 + extra > size - 1) {
      size *= 2;
      result = (char*)realloc(result, size);
    }

    if (len > 0) {
      strncat(result, " ", 1);
      len++;
    }

    strncat(result, *string, extra);
    len += extra;

    string++;
  }

  return result;
}

int parse_flags(int argc, char **argv) {
  int c;

  while ((c = getopt(argc, argv, "-:m:")) != -1) {
    switch (c) {
      case 'm':
        initial_flags.modes = optarg;
        break;
      default:
        printf("%d %s\n", optind, argv[optind]);
        // fprintf(stderr, "Usage: %s [-m modes] -- cmd [arg...]\n", argv[0]);
        // exit(EXIT_FAILURE);
        // break;
    }
  }

  while (*argv != NULL) {
    if (strcmp(*argv, "--") == 0) {
      break;
    }

    argv++;
  }

  initial_flags.cmd = join_strings(argv + 1);
}

void *write_to_stream(void *sptr) {
  Stream *s = (Stream*)sptr;
  Strbuf *sb = new_strbuf("");

  ssize_t n;
  char buf[128];

  StreamReader *sr = new_stream_reader(s);

  while ((n = stream_reader_read(sr, buf, 127))) {
    buf[n] = '\0';
    strbuf_write(sb, buf);
  }

  fprintf(stderr, "got: '%s'\n", sb->str);

  free_strbuf(sb);
  free_stream_reader(sr);
}


int main(int argc, char **argv) {
  parse_flags(argc, argv);

  // setlocale(LC_ALL, "");
  // initscr();
  // cbreak();
  // noecho();

  Stream *s = new_stream();
  stream_write(s, "1234");

  pthread_t threads[20];

  for (int i = 0; i < 20; i++) {
    pthread_create(&threads[i], NULL, write_to_stream, s);
  }

  stream_write(s, "567890");
  stream_write(s, "1234567890");
  stream_close(s);

  for (int i = 0; i < 20; i++) {
    pthread_join(threads[i], NULL);
  }

  free_stream(s);

  // endwin();

  return EXIT_SUCCESS;
}

