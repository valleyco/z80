/*
 * Convert TeleDisk .td0 images to Kaypro raw .dsk layouts.
 *
 * SSDD (Kaypro II / cpmtools kpii): 40 × 1 × 10 × 512 = 204800
 *   Sector IDs 0-9 on head 0. Track N at offset N*10*512.
 *
 * DSDD (Kaypro 4 / Universal): 40 × 2 × 10 × 512 = 409600
 *   Side 0 IDs 0-9, side 1 IDs 10-19.
 *   offset = ((cyl*2+head)*10+phys)*512
 *
 * --format auto (default): surfaces < 2 → ssdd, else dsdd.
 *
 * Uses td0Read.c (GPL-2+) from ogdenpm/disktools.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td0.h"
#include "utility.h"

#define KAYPRO_TRACKS 40
#define KAYPRO_SPT 10
#define KAYPRO_SSIZE 512
#define KAYPRO_SSDD_SIDES 1
#define KAYPRO_DSDD_SIDES 2
#define KAYPRO_SSDD_SIZE \
  (KAYPRO_TRACKS * KAYPRO_SSDD_SIDES * KAYPRO_SPT * KAYPRO_SSIZE)
#define KAYPRO_DSDD_SIZE \
  (KAYPRO_TRACKS * KAYPRO_DSDD_SIDES * KAYPRO_SPT * KAYPRO_SSIZE)

typedef enum { FMT_AUTO, FMT_SSDD, FMT_DSDD } format_t;

static bool verbose;
static bool debug_rejects;
static format_t out_format = FMT_AUTO;
static unsigned out_sides = KAYPRO_DSDD_SIDES;
static size_t out_size = KAYPRO_DSDD_SIZE;

static size_t kaypro_offset(unsigned cyl, unsigned head, unsigned phys) {
  return ((size_t)cyl * out_sides + head) * KAYPRO_SPT * KAYPRO_SSIZE +
         phys * KAYPRO_SSIZE;
}

static bool sector_usable(const sector_t *sec) {
  if ((sec->sSize & 0xf8) || (sec->flags & 0x30))
    return false;
  if (128u << sec->sSize != KAYPRO_SSIZE)
    return false;
  if (!sec->data)
    return false;
  return true;
}

/* Map sector ID to physical index 0..9 for the destination geometry. */
static int kaypro_phys_sec(unsigned head, unsigned id) {
  if (out_sides == KAYPRO_SSDD_SIDES) {
    if (head == 0 && id < KAYPRO_SPT)
      return (int)id;
    return -1;
  }
  if (head == 0 && id < KAYPRO_SPT)
    return (int)id;
  if (head == 1 && id >= 10 && id < 10 + KAYPRO_SPT)
    return (int)(id - 10);
  return -1;
}

static void write_sector(uint8_t *image, const sector_t *sec, unsigned track_cyl,
                         unsigned track_head, unsigned *written) {
  int phys = kaypro_phys_sec(track_head, sec->sec);
  unsigned cyl = track_cyl;
  unsigned head = track_head;

  if (phys < 0) {
    if (debug_rejects && sector_usable(sec))
      fprintf(stderr, "  reject cyl %u head %u sec %u (Kaypro sector id)\n", track_cyl,
              track_head, sec->sec);
    return;
  }
  if (cyl >= KAYPRO_TRACKS || head >= out_sides) {
    if (debug_rejects && sector_usable(sec))
      fprintf(stderr, "  reject cyl %u head %u sec %u (out of Kaypro range)\n", cyl, head,
              sec->sec);
    return;
  }
  if (!sector_usable(sec)) {
    if (debug_rejects)
      fprintf(stderr, "  reject cyl %u head %u sec %u sSize=%u flags=0x%02x data=%p\n",
              track_cyl, track_head, sec->sec, sec->sSize, sec->flags, (void *)sec->data);
    return;
  }

  size_t off = kaypro_offset(cyl, head, (unsigned)phys);
  memcpy(image + off, sec->data, KAYPRO_SSIZE);
  (*written)++;

  if (verbose)
    fprintf(stderr, "  src cyl %u head %u sec %u -> dst cyl %u head %u phys %d\n", track_cyl,
            track_head, sec->sec, cyl, head, phys);
}

static void usage(const char *prog) {
  fprintf(stderr, "Usage: %s [-v] [--format ssdd|dsdd|auto] input.td0 output.dsk\n", prog);
  fprintf(stderr, "  Convert TeleDisk image to Kaypro raw sector image.\n");
  fprintf(stderr, "  --format ssdd  40x1x10x512 (204800), linear SS (cpmtools kpii)\n");
  fprintf(stderr, "  --format dsdd  40x2x10x512 (409600), Universal DSDD\n");
  fprintf(stderr, "  --format auto  (default) surfaces<2 → ssdd, else dsdd\n");
}

static bool parse_format(const char *s, format_t *out) {
  if (strcmp(s, "ssdd") == 0) {
    *out = FMT_SSDD;
    return true;
  }
  if (strcmp(s, "dsdd") == 0) {
    *out = FMT_DSDD;
    return true;
  }
  if (strcmp(s, "auto") == 0) {
    *out = FMT_AUTO;
    return true;
  }
  return false;
}

static void apply_format(format_t fmt, unsigned surfaces) {
  format_t resolved = fmt;
  if (resolved == FMT_AUTO)
    resolved = (surfaces < 2) ? FMT_SSDD : FMT_DSDD;

  if (resolved == FMT_SSDD) {
    out_sides = KAYPRO_SSDD_SIDES;
    out_size = KAYPRO_SSDD_SIZE;
  } else {
    out_sides = KAYPRO_DSDD_SIDES;
    out_size = KAYPRO_DSDD_SIZE;
  }
}

int main(int argc, char **argv) {
  const char *in_path = NULL;
  const char *out_path = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      debug_rejects = true;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "--format") == 0) {
      if (i + 1 >= argc || !parse_format(argv[++i], &out_format)) {
        usage(argv[0]);
        return 1;
      }
    } else if (strncmp(argv[i], "--format=", 9) == 0) {
      if (!parse_format(argv[i] + 9, &out_format)) {
        usage(argv[0]);
        return 1;
      }
    } else if (!in_path) {
      in_path = argv[i];
    } else if (!out_path) {
      out_path = argv[i];
    } else {
      usage(argv[0]);
      return 1;
    }
  }

  if (!in_path || !out_path) {
    usage(argv[0]);
    return 1;
  }

  td0Header_t *hdr = openTd0((char *)in_path);
  apply_format(out_format, hdr->surfaces);

  const char *geom_name = (out_sides == KAYPRO_SSDD_SIDES) ? "ssdd" : "dsdd";
  fprintf(stderr, "TD0: ver %d.%d sig=%s surfaces=%u compression=%s\n", hdr->ver / 10,
          hdr->ver % 10, hdr->sig, hdr->surfaces,
          hdr->sig[0] == 'T' ? "none" : hdr->ver > 19 ? "LZHUF" : "LZW");
  fprintf(stderr, "Geometry: %s (%u cyl × %u side%s × %u × %u = %zu bytes)\n", geom_name,
          KAYPRO_TRACKS, out_sides, out_sides == 1 ? "" : "s", KAYPRO_SPT, KAYPRO_SSIZE,
          out_size);

  uint8_t *image = calloc(1, out_size);
  if (!image)
    fatal("Out of memory");

  unsigned written = 0;
  td0Track_t *track;
  while ((track = readTrack()) != NULL) {
    if (verbose)
      fprintf(stderr, "track cyl=%u head=%u nSec=%u\n", track->cyl, track->head & 0x7fu,
              track->nSec);
    for (unsigned i = 0; i < track->nSec; i++)
      write_sector(image, &track->sectors[i], track->cyl, track->head & 0x7fu, &written);
  }
  closeTd0();

  if (written == 0)
    fatal("No sectors written from %s", in_path);

  FILE *out = fopen(out_path, "wb");
  if (!out)
    fatal("Cannot create %s", out_path);
  if (fwrite(image, 1, out_size, out) != out_size)
    fatal("Write failed on %s", out_path);
  fclose(out);
  free(image);

  fprintf(stderr, "Wrote %s (%u sectors, %zu bytes)\n", out_path, written, out_size);
  return 0;
}
