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

void *input_loop(void *fdp) {
  int fd = *((int*)fdp);

  while (1) {
    getch();
    write(fd, "f", 1); // TODO: Should non-block?
  }
}

int main(int argc, char **argv) {
  int fds[2];
  pthread_t input_thread;

  parse_flags(argc, argv);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();

  refresh();

  Pager *p = new_pager("git diff --no-color");

  pipe(fds);

  pthread_create(&input_thread, NULL, input_loop, &fds[1]);

  struct pollfd pfds[1];
  pfds[0].fd = fds[0];
  pfds[0].events = POLLIN;

  int done = 0;
  char foo[1];
  int retval;

  while (!done) {
    retval = poll(pfds, 1, 33);

    if (retval == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    } else if (retval) {
      if (pfds[0].revents & POLLIN) {
        read(fds[0], foo, 1);
      }

      fprintf(stderr, "got input\n");
    } else {
      render_pager(p);
    }
  }

  endwin();

  return EXIT_SUCCESS;
}

