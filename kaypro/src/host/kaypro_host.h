#ifndef KAYPRO_HOST_H
#define KAYPRO_HOST_H

#include <stddef.h>
#include <stdint.h>

#include "kaypro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Host I/O callbacks. NULL function pointers = silent / no input. */
typedef struct kaypro_host_ops {
  void (*console_write)(void *ctx, const uint8_t *data, size_t len);
  int (*console_poll)(void *ctx); /* -1 if none, else byte for SIO B */
  void (*log)(void *ctx, const char *msg);
  void *ctx;
} kaypro_host_ops_t;

void kaypro_set_host(kaypro_t *m, const kaypro_host_ops_t *ops);

/* Path-based loaders (POSIX host). Prefer blob APIs on freestanding targets. */
bool kaypro_load_rom(kaypro_t *m, const char *path);
bool kaypro_attach_disk(kaypro_t *m, int drive, const char *path);

/* Install stdin/stdout/stderr console + log ops for desktop runs. */
void kaypro_host_posix_install(kaypro_t *m);

#ifdef __cplusplus
}
#endif

#endif /* KAYPRO_HOST_H */
