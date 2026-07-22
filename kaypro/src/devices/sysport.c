#include <stdlib.h>

#include "kaypro_internal.h"
#include "fdc1793.h"
#include "sysport.h"

typedef struct {
  EmuDevice emu;
  kaypro_t *machine;
  kaypro_fdc_t *fdc;
  uint8_t latch;
} sysport_impl_t;

static int sysport_read(void *dev, int port) {
  sysport_impl_t *sys = (sysport_impl_t *)dev;
  (void)port;
  return sys->latch;
}

static void sysport_write(void *dev, int port, int value) {
  sysport_impl_t *sys = (sysport_impl_t *)dev;
  (void)port;
  sys->latch = (uint8_t)value;
  sys->machine->bank1 = (sys->latch & 0x80) != 0;
  if (sys->fdc) fdc1793_set_sysport(sys->fdc, sys->latch);
}

static void sysport_reset(EmuDevice *dev) {
  sysport_impl_t *sys = (sysport_impl_t *)dev->ctx;
  sys->latch = 0x80; /* bank1 selected at reset */
  kaypro_sysport_sync_bank((kaypro_sysport_t *)sys);
}

static void sysport_destroy(EmuDevice *dev) { free(dev->ctx); }

static int (*sysport_reads[])(void *, int) = {sysport_read};
static void (*sysport_writes[])(void *, int, int) = {sysport_write};

kaypro_sysport_t *kaypro_sysport_create(kaypro_t *machine, kaypro_fdc_t *fdc) {
  sysport_impl_t *sys = (sysport_impl_t *)calloc(1, sizeof(sysport_impl_t));
  if (!sys) return NULL;

  sys->machine = machine;
  sys->fdc = fdc;
  sys->latch = 0x80;
  sys->emu.ctx = sys;
  sys->emu.port.port_count = 1;
  sys->emu.port.read = sysport_reads;
  sys->emu.port.write = sysport_writes;
  sys->emu.reset = sysport_reset;
  sys->emu.destroy = sysport_destroy;

  kaypro_sysport_sync_bank((kaypro_sysport_t *)sys);
  return (kaypro_sysport_t *)sys;
}

void kaypro_sysport_sync_bank(kaypro_sysport_t *sysport) {
  sysport_impl_t *sys = (sysport_impl_t *)sysport;
  sys->machine->bank1 = (sys->latch & 0x80) != 0;
  if (sys->fdc) fdc1793_set_sysport(sys->fdc, sys->latch);
}

uint8_t kaypro_sysport_latch(const kaypro_sysport_t *sysport) {
  return ((const sysport_impl_t *)sysport)->latch;
}

void kaypro_sysport_set_latch(kaypro_sysport_t *sysport, uint8_t latch) {
  sysport_impl_t *sys = (sysport_impl_t *)sysport;
  sys->latch = latch;
  kaypro_sysport_sync_bank(sysport);
}
