#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "hdc_wd1002.h"
#include "kaypro.h"
#include "kaypro_internal.h"

#define PORT_HDC_ERROR 0x81
#define PORT_HDC_STATUS 0x87
#define PORT_HDC_CMD 0x87

static uint8_t read_port(kaypro_t *m, uint16_t port) {
  return kaypro_bus_io_read(m, port);
}

static void write_port(kaypro_t *m, uint16_t port, uint8_t value) {
  kaypro_bus_io_write(m, port, value);
}

int main(void) {
  kaypro_t *m = kaypro_create();
  assert(m);

  /* Absent controller: not busy, error != 0/01 so winrest fails fast. */
  assert((read_port(m, PORT_HDC_STATUS) & 0x80) == 0);
  assert(read_port(m, PORT_HDC_ERROR) != 0x00);
  assert(read_port(m, PORT_HDC_ERROR) != 0x01);

  write_port(m, PORT_HDC_CMD, 0x10); /* restore */
  assert((read_port(m, PORT_HDC_STATUS) & 0x80) == 0);
  assert(read_port(m, PORT_HDC_ERROR) != 0x00);
  assert(read_port(m, PORT_HDC_ERROR) != 0x01);

  kaypro_destroy(m);
  printf("kaypro hdc smoke: ok\n");
  return 0;
}
