#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "utility.h"

_Noreturn void fatal(char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fputs("Error: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
  exit(1);
}

void warn(char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fputs("Warning: ", stderr);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
}

void *safeMalloc(size_t size) {
  void *p = malloc(size);
  if (!p)
    fatal("Out of memory");
  return p;
}

char const *basename_path(char const *path) {
  char const *base = path;
  for (char const *p = path; *p; p++) {
    if (*p == '/' || *p == '\\')
      base = p + 1;
  }
  return base;
}
