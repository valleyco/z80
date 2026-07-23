#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "kaypro.h"
#include "kaypro_host.h"
#include "kaypro_internal.h"
#include "sysport.h"
#include "z80.h"

#define ROM_SMOKE_PATH "tests/rom_smoke.bin"
#define ROM_SMOKE_EXPECTED_LATCH 0xB6

static long file_size(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) return -1;
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return -1;
  }
  long size = ftell(file);
  fclose(file);
  return size;
}

int main(void) {
  long rom_size = file_size(ROM_SMOKE_PATH);
  assert(rom_size > 0);
  assert(rom_size <= KAYPRO_ROM_SIZE);

  kaypro_t *m = kaypro_create();
  assert(m);
  assert(kaypro_load_rom(m, ROM_SMOKE_PATH));

  z80_set_fetch_guard(&m->cpu, 0x0000, (uint16_t)rom_size);
  kaypro_reset(m);

  for (unsigned step = 0; step < 64; step++) {
    if (m->cpu.halted || z80_fetch_trap(&m->cpu)) break;
    kaypro_step(m, 1);
  }

  assert(m->cpu.halted);
  assert(!z80_fetch_trap(&m->cpu));
  assert(kaypro_sysport_latch(m->sysport) == ROM_SMOKE_EXPECTED_LATCH);
  assert(m->bank1);

  kaypro_destroy(m);
  printf("kaypro rom smoke: ok\n");
  return 0;
}