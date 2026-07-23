/*
 * Convert TeleDisk .td0 images to Kaypro 4/84 DSDD raw .dsk layout.
 *
 * Universal ROM DSDD uses 10 x 512-byte sectors per side (DPB SPT=40 records):
 *   side 0 IDs 0-9, side 1 IDs 10-19. Sector 0 is the cold-boot record.
 * Linear layout: track-major, side 0 then side 1:
 *   offset = ((cyl * 2 + head) * 10 + phys) * 512
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
#define KAYPRO_SIDES 2
#define KAYPRO_SPT 10
#define KAYPRO_SSIZE 512
#define KAYPRO_DSDD_SIZE (KAYPRO_TRACKS * KAYPRO_SIDES * KAYPRO_SPT * KAYPRO_SSIZE)

static bool verbose;
static bool debug_rejects;

static size_t kaypro_offset(unsigned cyl, unsigned head, unsigned phys) {
  return ((size_t)cyl * KAYPRO_SIDES + head) * KAYPRO_SPT * KAYPRO_SSIZE +
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

static int kaypro_phys_sec(unsigned head, unsigned id) {
  if (head == 0 && id < KAYPRO_SPT)
    return (int)id;
  if (head == 1 && id >= 10 && id < 10 + KAYPRO_SPT)
    return (int)(id - 10);
  return -1;
}

static void write_sector(uint8_t *image, const sector_t *sec, unsigned track_cyl,
                         unsigned track_head, unsigned *written) {
  unsigned cyl = track_cyl;
  unsigned head = track_head;
  int phys = kaypro_phys_sec(head, sec->sec);

  if (phys < 0) {
    if (debug_rejects && sector_usable(sec))
      fprintf(stderr, "  reject cyl %u head %u sec %u (Kaypro sector id)\n", cyl, head, sec->sec);
    return;
  }
  if (cyl >= KAYPRO_TRACKS || head >= KAYPRO_SIDES) {
    if (debug_rejects && sector_usable(sec))
      fprintf(stderr, "  reject cyl %u head %u sec %u (out of Kaypro range)\n", cyl, head, sec->sec);
    return;
  }
  if (!sector_usable(sec)) {
    if (debug_rejects)
      fprintf(stderr, "  reject cyl %u head %u sec %u sSize=%u flags=0x%02x data=%p\n", cyl, head,
              sec->sec, sec->sSize, sec->flags, (void *)sec->data);
    return;
  }

  size_t off = kaypro_offset(cyl, head, (unsigned)phys);
  memcpy(image + off, sec->data, KAYPRO_SSIZE);
  (*written)++;

  if (verbose)
    fprintf(stderr, "  cyl %u head %u sec %u -> phys %d\n", cyl, head, sec->sec, phys);
}

static void usage(const char *prog) {
  fprintf(stderr, "Usage: %s [-v] input.td0 output.dsk\n", prog);
  fprintf(stderr, "  Convert TeleDisk image to Kaypro DSDD raw sector image (40x2x10x512).\n");
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
  if (verbose) {
    fprintf(stderr, "TD0: ver %d.%d sig=%s surfaces=%u compression=%s\n",
            hdr->ver / 10, hdr->ver % 10, hdr->sig,
            hdr->surfaces,
            hdr->sig[0] == 'T' ? "none"
            : hdr->ver > 19      ? "LZHUF"
                                 : "LZW");
  }

  uint8_t *image = calloc(1, KAYPRO_DSDD_SIZE);
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

  FILE *out = fopen(out_path, "wb");
  if (!out)
    fatal("Cannot create %s", out_path);
  if (fwrite(image, 1, KAYPRO_DSDD_SIZE, out) != KAYPRO_DSDD_SIZE)
    fatal("Write failed on %s", out_path);
  fclose(out);
  free(image);

  fprintf(stderr, "Wrote %s (%u sectors, %u bytes)\n", out_path, written, KAYPRO_DSDD_SIZE);
  return 0;
}
