#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "kaypro.h"
#include "kaypro_internal.h"
#include "sysport.h"

int main(void) {
  kaypro_t *m = kaypro_create();
  assert(m);

  /* Fill ROM with a recognizable pattern */
  for (int i = 0; i < 256; i++) m->rom[i] = (uint8_t)(0xA0 + (i & 0x0F));

  kaypro_set_bank1(m, true);
  assert(kaypro_mem_read(m, 0x0000) == 0xA0);
  assert(kaypro_mem_read(m, 0x00FF) == 0xAF);

  kaypro_set_bank1(m, false);
  m->ram[0x0100] = 0x55;
  assert(kaypro_mem_read(m, 0x0100) == 0x55);

  /* Bank 1: low 16K ROM, upper addresses still RAM */
  kaypro_set_bank1(m, true);
  m->ram[0x8000] = 0x42;
  assert(kaypro_mem_read(m, 0x8000) == 0x42);

  kaypro_destroy(m);
  printf("kaypro smoke: ok\n");
  return 0;
}
