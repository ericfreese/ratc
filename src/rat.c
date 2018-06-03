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

int main(int argc, char **argv) {
  parse_flags(argc, argv);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();

  printw("creating buffer...\n");
  refresh();

  new_buffer(STDIN_FILENO);
  sleep(20);

  endwin();

  return EXIT_SUCCESS;
}

