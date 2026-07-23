#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "crt6845.h"
#include "kaypro.h"
#include "kaypro_internal.h"

/* Universal / Kaypro-10 SY6545 map (urmtkit1 VIDEO.MAC). */
#define PORT_CRT_STATUS 0x1C
#define PORT_CRT_CMD 0x1C
#define PORT_CRT_REGDATA 0x1D
#define PORT_CRT_DATA 0x1F

#define CRT_REG_ADDR_HI 0x12
#define CRT_REG_ADDR_LO 0x13
#define CRT_ATTR_PLANE 0x08

static uint8_t read_port(kaypro_t *m, uint16_t port) {
  return kaypro_bus_io_read(m, port);
}

static void write_port(kaypro_t *m, uint16_t port, uint8_t value) {
  kaypro_bus_io_write(m, port, value);
}

static void crt_select_reg(kaypro_t *m, uint8_t reg) {
  write_port(m, PORT_CRT_CMD, reg);
}

static void crt_write_reg(kaypro_t *m, uint8_t reg, uint8_t value) {
  crt_select_reg(m, reg);
  write_port(m, PORT_CRT_REGDATA, value);
}

static uint8_t crt_read_reg(kaypro_t *m, uint8_t reg) {
  crt_select_reg(m, reg);
  return read_port(m, PORT_CRT_REGDATA);
}

static void crt_set_addr(kaypro_t *m, uint16_t addr, bool attr_plane) {
  uint8_t hi = (uint8_t)((addr >> 8) & 0x07);
  if (attr_plane) hi |= CRT_ATTR_PLANE;
  crt_write_reg(m, CRT_REG_ADDR_HI, hi);
  crt_write_reg(m, CRT_REG_ADDR_LO, (uint8_t)(addr & 0xFF));
}

static void crt_strobe(kaypro_t *m) {
  /* ROM selects register 1Fh before data cycles ("tickle"). */
  write_port(m, PORT_CRT_CMD, 0x1F);
}

int main(void) {
  kaypro_t *m = kaypro_create();
  assert(m);

  /* Status bit7 must be set or vidinit clear loops forever. */
  assert((read_port(m, PORT_CRT_STATUS) & 0x80) != 0);

  /* Controller programming: select reg, write, read back. */
  crt_write_reg(m, 0x00, 0x6A);
  crt_write_reg(m, 0x01, 0x50);
  assert(crt_read_reg(m, 0x00) == 0x6A);
  assert(crt_read_reg(m, 0x01) == 0x50);

  /* Char-plane writes echo printable text (TX capture; no host console). */
  kaypro_crt_tx_clear(m->crt);
  crt_set_addr(m, 0x0000, false);
  crt_strobe(m);
  assert((read_port(m, PORT_CRT_STATUS) & 0x80) != 0);
  write_port(m, PORT_CRT_DATA, 'H');
  write_port(m, PORT_CRT_DATA, 'i');
  write_port(m, PORT_CRT_DATA, '\r');
  assert(kaypro_crt_tx_count(m->crt) == 3);
  assert(kaypro_crt_tx_at(m->crt, 0) == 'H');
  assert(kaypro_crt_tx_at(m->crt, 1) == 'i');
  assert(kaypro_crt_tx_at(m->crt, 2) == '\n');

  /* Leading spaces (clear-screen flood) are suppressed. */
  kaypro_crt_tx_clear(m->crt);
  write_port(m, PORT_CRT_DATA, ' ');
  write_port(m, PORT_CRT_DATA, ' ');
  assert(kaypro_crt_tx_count(m->crt) == 0);

  /* Inter-word space after a graphic is kept. */
  write_port(m, PORT_CRT_DATA, 'A');
  write_port(m, PORT_CRT_DATA, ' ');
  write_port(m, PORT_CRT_DATA, 'B');
  assert(kaypro_crt_tx_count(m->crt) == 3);
  assert(kaypro_crt_tx_at(m->crt, 0) == 'A');
  assert(kaypro_crt_tx_at(m->crt, 1) == ' ');
  assert(kaypro_crt_tx_at(m->crt, 2) == 'B');

  /* Attribute-plane writes must not echo. */
  kaypro_crt_tx_clear(m->crt);
  crt_set_addr(m, 0x0001, true);
  crt_strobe(m);
  write_port(m, PORT_CRT_DATA, 'X');
  assert(kaypro_crt_tx_count(m->crt) == 0);

  kaypro_destroy(m);
  printf("kaypro crt smoke: ok\n");
  return 0;
}
