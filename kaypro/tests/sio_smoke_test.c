#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "kaypro.h"
#include "kaypro_internal.h"
#include "sio.h"

#define PORT_SIO_B_DATA 0x05
#define PORT_SIO_B_CTRL 0x07

static uint8_t read_port(kaypro_t *m, uint16_t port) {
  return kaypro_bus_io_read(m, port);
}

int main(void) {
  kaypro_t *m = kaypro_create();
  assert(m);

  assert(read_port(m, PORT_SIO_B_CTRL) == 0x04);

  kaypro_sio_push_rx_b(m->sio, 'A');
  assert(read_port(m, PORT_SIO_B_CTRL) == 0x05);
  assert(read_port(m, PORT_SIO_B_DATA) == 'A');
  assert(read_port(m, PORT_SIO_B_CTRL) == 0x04);

  kaypro_destroy(m);
  printf("kaypro sio smoke: ok\n");
  return 0;
}