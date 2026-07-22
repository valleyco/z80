#include "kaypro_internal.h"

uint8_t kaypro_bus_mem_read(void *ctx, uint16_t addr) {
  kaypro_t *m = (kaypro_t *)ctx;
  if (m->bank1 && addr < 0x4000) {
    if (addr < KAYPRO_ROM_SIZE) return m->rom[addr];
    return 0xFF;
  }
  return m->ram[addr];
}

void kaypro_bus_mem_write(void *ctx, uint16_t addr, uint8_t v) {
  kaypro_t *m = (kaypro_t *)ctx;
  if (m->bank1 && addr < 0x4000) {
    if (addr < KAYPRO_ROM_SIZE) return;
    return;
  }
  m->ram[addr] = v;
}

void kaypro_mem_init(kaypro_t *m) {
  for (unsigned i = 0; i < sizeof(m->ram); i++) m->ram[i] = 0;
  for (unsigned i = 0; i < sizeof(m->rom); i++) m->rom[i] = 0xFF;
}
