#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kaypro.h"
#include "kaypro_host.h"

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s --rom FILE [--disk-a FILE] [--disk-b FILE] [--trace] [--mcycles N] [--chunk N]\n",
          prog);
}

static bool parse_u32(const char *text, unsigned *value) {
  char *end = NULL;
  unsigned long parsed = strtoul(text, &end, 0);
  if (!text[0] || (end && *end) || parsed > 0xffffffffUL) return false;
  *value = (unsigned)parsed;
  return true;
}

int main(int argc, char **argv) {
  const char *rom_path = NULL;
  const char *disk_a = NULL;
  const char *disk_b = NULL;
  bool trace = false;
  unsigned limit_mcycles = 0;
  unsigned chunk_mcycles = 10000;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
      rom_path = argv[++i];
    } else if (strcmp(argv[i], "--disk-a") == 0 && i + 1 < argc) {
      disk_a = argv[++i];
    } else if (strcmp(argv[i], "--disk-b") == 0 && i + 1 < argc) {
      disk_b = argv[++i];
    } else if (strcmp(argv[i], "--trace") == 0) {
      trace = true;
    } else if (strcmp(argv[i], "--mcycles") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &limit_mcycles) || limit_mcycles == 0) {
        fprintf(stderr, "Invalid --mcycles value: %s\n", argv[i]);
        return 1;
      }
    } else if (strcmp(argv[i], "--chunk") == 0 && i + 1 < argc) {
      if (!parse_u32(argv[++i], &chunk_mcycles) || chunk_mcycles == 0) {
        fprintf(stderr, "Invalid --chunk value: %s\n", argv[i]);
        return 1;
      }
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

  kaypro_host_posix_install(m);

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

  fprintf(stderr,
          "Kaypro 4/84 emulator running (Ctrl-\\ to quit; Ctrl-C goes to CP/M)\n");

  if (limit_mcycles > 0) {
    unsigned ran = 0;
    while (ran < limit_mcycles && !kaypro_halted(m) && !kaypro_fetch_trap_hit(m) &&
           !kaypro_host_posix_quit_requested()) {
      unsigned slice = chunk_mcycles;
      if (slice > limit_mcycles - ran) slice = limit_mcycles - ran;
      kaypro_step(m, slice);
      ran += slice;
    }
    fprintf(stderr,
            "Stopped after %u m-cycles: pc=%04X halted=%u trap=%u trap_pc=%04X sysport=%02X\n",
            ran, kaypro_pc(m), kaypro_halted(m), kaypro_fetch_trap_hit(m),
            kaypro_fetch_trap_addr(m), kaypro_sysport_state(m));
  } else {
    while (!kaypro_host_posix_quit_requested()) {
      kaypro_step(m, chunk_mcycles);
      /* Yield so the terminal can process PTY I/O; a tight spin can make
       * interactive sessions look frozen after heavy ANSI screen paints. */
      poll(NULL, 0, 1); /* 1ms */
    }
  }

  kaypro_destroy(m);
  return 0;
}
