#include "rat.h"

int main(int argc, char **argv) {
  rat_init();
  rat_run();
  rat_cleanup();

  return EXIT_SUCCESS;
}

