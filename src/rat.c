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

typedef struct {
  int fd;
  InputQueue *q;
} InputLoopParams;

void *input_loop(void *ilpp) {
  InputLoopParams *ilp = ((InputLoopParams*)ilpp);

  while (1) {
    enqueue_input(ilp->q, getch());
    write(ilp->fd, "f", 1);
  }
}

void main_loop(InputQueue *q, int fd) {
  int done = 0;
  char dummy[1];
  int retval;
  KeyStack *key_stack = new_key_stack();

  struct pollfd input_pfds[1];
  input_pfds[0].fd = fd;
  input_pfds[0].events = POLLIN;

  Pager *p = new_pager("git diff --no-color");

  while (!done) {
    retval = poll(input_pfds, 1, 33);

    if (retval == -1) {
      // Handle interrupted syscall
      if (errno == EINTR) {
        continue;
      }

      perror("poll");
      exit(EXIT_FAILURE);
    } else if (retval && input_pfds[0].revents & POLLIN) {
      while (read(input_pfds[0].fd, dummy, 1) > 0) {
        int ch = dequeue_input(q);
        key_stack_push(key_stack, (char*)keyname(ch));

        Strbuf *sb = new_strbuf("");
        key_stack_to_strbuf(key_stack, sb);
        fprintf(stderr, "keystack: '%s'\n", sb->str);
        free_strbuf(sb);
      }

      if (errno != EAGAIN) {
        perror("read");
        exit(EXIT_FAILURE);
      }
    }

    render_pager(p);
  }
}

int main(int argc, char **argv) {
  parse_flags(argc, argv);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();

  InputQueue *input_queue = new_input_queue();
  pthread_t input_thread;
  InputLoopParams ilp;
  int input_fds[2];

  pipe(input_fds);
  fcntl(input_fds[0], F_SETFL, O_NONBLOCK);

  ilp.fd = input_fds[1];
  ilp.q = input_queue;

  pthread_create(&input_thread, NULL, input_loop, &ilp);

  main_loop(input_queue, input_fds[0]);

  endwin();

  return EXIT_SUCCESS;
}

