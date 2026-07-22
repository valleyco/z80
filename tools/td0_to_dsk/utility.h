#pragma once
#include <stdio.h>
#include <stddef.h>

_Noreturn void fatal(char const *fmt, ...);
void warn(char const *fmt, ...);
void *safeMalloc(size_t size);
char const *basename_path(char const *path);
