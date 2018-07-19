#include "util.h"

char *copy_string(char *str) {
  size_t len = strlen(str);
  char *copy = malloc(sizeof(*copy) * len + 1);
  memcpy(copy, str, len);
  copy[len] = '\0';
  return copy;
}
