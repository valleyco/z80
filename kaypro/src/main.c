#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kaypro.h"

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s --rom FILE [--disk-a FILE] [--disk-b FILE] [--trace]\n",
          prog);
}

int main(int argc, char **argv) {
  const char *rom_path = NULL;
  const char *disk_a = NULL;
  const char *disk_b = NULL;
  bool trace = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
      rom_path = argv[++i];
    } else if (strcmp(argv[i], "--disk-a") == 0 && i + 1 < argc) {
      disk_a = argv[++i];
    } else if (strcmp(argv[i], "--disk-b") == 0 && i + 1 < argc) {
      disk_b = argv[++i];
    } else if (strcmp(argv[i], "--trace") == 0) {
      trace = true;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  if (!rom_path) {
    usage(argv[0]);
    return 1;
  }

  kaypro_t *m = kaypro_create();
  if (!m) {
    fprintf(stderr, "Failed to create machine\n");
    return 1;
  }

  if (!kaypro_load_rom(m, rom_path)) {
    fprintf(stderr, "Failed to load ROM: %s\n", rom_path);
    kaypro_destroy(m);
    return 1;
  }

  if (disk_a && !kaypro_attach_disk(m, 0, disk_a)) {
    fprintf(stderr, "Failed to load disk A: %s\n", disk_a);
    kaypro_destroy(m);
    return 1;
  }
  if (disk_b && !kaypro_attach_disk(m, 1, disk_b)) {
    fprintf(stderr, "Failed to load disk B: %s\n", disk_b);
    kaypro_destroy(m);
    return 1;
  }

  kaypro_set_trace_io(m, trace);
  kaypro_reset(m);

  fprintf(stderr, "Kaypro 4/84 emulator running (Ctrl-C to quit)\n");
  while (1) {
    kaypro_step(m, 10000);
  }

  kaypro_destroy(m);
  return 0;
}
