#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kaypro_host.h"

static bool stdin_nonblock_set;

static void posix_console_write(void *ctx, const uint8_t *data, size_t len) {
  (void)ctx;
  if (!data || len == 0) return;
  fwrite(data, 1, len, stdout);
  fflush(stdout);
}

static void posix_ensure_stdin_nonblock(void) {
  if (stdin_nonblock_set) return;
  int fd = STDIN_FILENO;
  if (fd >= 0) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
  stdin_nonblock_set = true;
}

static int posix_console_poll(void *ctx) {
  (void)ctx;
  posix_ensure_stdin_nonblock();
  int c = getchar();
  if (c == EOF) return -1;
  return c & 0x7F;
}

static void posix_log(void *ctx, const char *msg) {
  (void)ctx;
  if (msg) fprintf(stderr, "%s\n", msg);
}

void kaypro_host_posix_install(kaypro_t *m) {
  kaypro_host_ops_t ops = {
      .console_write = posix_console_write,
      .console_poll = posix_console_poll,
      .log = posix_log,
      .ctx = NULL,
  };
  kaypro_set_host(m, &ops);
}

static uint8_t *posix_read_file(const char *path, size_t *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long sz = ftell(f);
  if (sz <= 0) {
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
    fclose(f);
    free(buf);
    return NULL;
  }
  fclose(f);
  *out_size = (size_t)sz;
  return buf;
}

bool kaypro_load_rom(kaypro_t *m, const char *path) {
  size_t size = 0;
  uint8_t *data = posix_read_file(path, &size);
  if (!data) return false;
  bool ok = kaypro_load_rom_bytes(m, data, size);
  free(data);
  return ok;
}

bool kaypro_attach_disk(kaypro_t *m, int drive, const char *path) {
  size_t size = 0;
  uint8_t *data = posix_read_file(path, &size);
  if (!data) return false;
  /* FDC takes ownership of data. */
  if (!kaypro_attach_disk_mem(m, drive, data, size)) {
    free(data);
    return false;
  }
  return true;
}
