#include <stdio.h>
#include <stdlib.h>

#include "kaypro_internal.h"
#include "crt6845.h"
#include "fdc1793.h"
#include "keyboard.h"
#include "sio.h"
#include "sysport.h"

void kaypro_add_device(kaypro_t *m, EmuDevice *dev) {
  if (!dev) return;
  if (m->device_count >= m->device_cap) {
    int cap = m->device_cap ? m->device_cap * 2 : 8;
    EmuDevice **next = (EmuDevice **)realloc(m->devices, (size_t)cap * sizeof(EmuDevice *));
    if (!next) return;
    m->devices = next;
    m->device_cap = cap;
  }
  m->devices[m->device_count++] = dev;
}

void kaypro_register_ports(kaypro_t *m) {
  port_bus_init(&m->port_bus);
  /* Universal board map (urmtkit1): SIO 04-07, FDC 10-13, bitport 14, CRT 1C-1F. */
  port_bus_register(&m->port_bus, &m->sio->emu.port, 0x04);
  port_bus_register(&m->port_bus, &m->fdc->emu.port, 0x10);
  port_bus_register(&m->port_bus, &m->sysport->emu.port, 0x14);
  port_bus_register(&m->port_bus, &m->crt->emu.port, 0x1C);
}

uint8_t kaypro_bus_io_read(void *ctx, uint16_t port) {
  kaypro_t *m = (kaypro_t *)ctx;
  uint8_t v = port_bus_read(&m->port_bus, port);
  if (m->trace_io) {
    fprintf(stderr, "IN  %04X -> %02X\n", port, v);
  }
  return v;
}

void kaypro_bus_io_write(void *ctx, uint16_t port, uint8_t v) {
  kaypro_t *m = (kaypro_t *)ctx;
  if (m->trace_io) {
    fprintf(stderr, "OUT %04X <- %02X\n", port, v);
  }
  /* Baud rate latches (not on FDC). */
  uint8_t p = (uint8_t)(port & 0xFF);
  if (p == 0x00 || p == 0x08) {
    kaypro_sio_set_baud(m->sio, v);
  }
  port_bus_write(&m->port_bus, port, v);
}
