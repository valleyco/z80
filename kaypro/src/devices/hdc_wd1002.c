#include <stdlib.h>
#include <string.h>

#include "hdc_wd1002.h"

/*
 * Kaypro 10 WD1002-HD0 ports (ROM19EE / MAME):
 *   80 data, 81 error/precomp, 82 sec count, 83 sector,
 *   84 cyl lo, 85 cyl hi, 86 SDH, 87 status/command.
 *
 * diskinit winrest treats error register 00h/01h as "controller present"
 * (01h = expected WD2797-absent self-test on HDO). Any other nonzero error
 * with BUSY clear makes restore fail immediately and fall back to floppies.
 */
#define HDC_ERR_BUS 0x04

typedef struct {
  EmuDevice emu;
  uint8_t regs[8];
  uint8_t error;
  uint8_t status;
} hdc_impl_t;

static int hdc_read(void *dev, int port) {
  hdc_impl_t *hdc = (hdc_impl_t *)dev;
  switch (port) {
    case 1:
      return hdc->error;
    case 7:
      return hdc->status;
    default:
      if (port >= 0 && port < 8) return hdc->regs[port];
      return 0;
  }
}

static void hdc_write(void *dev, int port, int value) {
  hdc_impl_t *hdc = (hdc_impl_t *)dev;
  uint8_t v = (uint8_t)value;
  if (port < 0 || port >= 8) return;
  if (port == 7) {
    /* Ignore commands; stay not-present. */
    hdc->status = 0x00;
    hdc->error = HDC_ERR_BUS;
    return;
  }
  if (port == 1) {
    hdc->regs[1] = v; /* write precomp */
    return;
  }
  hdc->regs[port] = v;
}

static void hdc_reset(EmuDevice *dev) {
  hdc_impl_t *hdc = (hdc_impl_t *)dev->ctx;
  memset(hdc->regs, 0, sizeof(hdc->regs));
  hdc->status = 0x00; /* not busy, not ready */
  hdc->error = HDC_ERR_BUS;
}

static void hdc_destroy(EmuDevice *dev) { free(dev->ctx); }

static int (*hdc_reads[])(void *, int) = {
    hdc_read, hdc_read, hdc_read, hdc_read,
    hdc_read, hdc_read, hdc_read, hdc_read,
};

static void (*hdc_writes[])(void *, int, int) = {
    hdc_write, hdc_write, hdc_write, hdc_write,
    hdc_write, hdc_write, hdc_write, hdc_write,
};

kaypro_hdc_t *kaypro_hdc_create(void) {
  hdc_impl_t *hdc = (hdc_impl_t *)calloc(1, sizeof(hdc_impl_t));
  if (!hdc) return NULL;

  hdc->emu.ctx = hdc;
  hdc->emu.port.port_count = 8;
  hdc->emu.port.read = hdc_reads;
  hdc->emu.port.write = hdc_writes;
  hdc->emu.reset = hdc_reset;
  hdc->emu.destroy = hdc_destroy;
  hdc_reset(&hdc->emu);
  return (kaypro_hdc_t *)hdc;
}
