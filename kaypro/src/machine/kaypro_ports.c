#include <stdlib.h>
#include <string.h>

#include "kaypro_internal.h"
#include "crt6845.h"
#include "fdc1793.h"
#include "hdc_wd1002.h"
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
  /* Universal board map (urmtkit1): SIO 04-07, FDC 10-13, bitport 14, CRT 1C-1F,
   * WD1002-HD0 80-87. */
  port_bus_register(&m->port_bus, &m->sio->emu.port, 0x04);
  port_bus_register(&m->port_bus, &m->fdc->emu.port, 0x10);
  port_bus_register(&m->port_bus, &m->sysport->emu.port, 0x14);
  port_bus_register(&m->port_bus, &m->crt->emu.port, 0x1C);
  port_bus_register(&m->port_bus, &m->hdc->emu.port, 0x80);
}

static void hex_nibble(char *out, unsigned v) {
  static const char *digits = "0123456789ABCDEF";
  *out = digits[v & 0xF];
}

static void hex_byte(char *out, unsigned v) {
  hex_nibble(out, v >> 4);
  hex_nibble(out + 1, v);
}

static void hex_word(char *out, unsigned v) {
  hex_byte(out, v >> 8);
  hex_byte(out + 2, v);
}

static void kaypro_trace_log(kaypro_t *m, bool is_out, uint16_t port, uint8_t value) {
  if (!m->host.log) return;
  char msg[48];
  /* "PC=XXXX IN  XX (XXXX) -> XX" / "PC=XXXX OUT XX (XXXX) <- XX" */
  memcpy(msg, "PC=", 3);
  hex_word(msg + 3, m->cpu.pc);
  if (is_out) {
    memcpy(msg + 7, " OUT ", 5);
    hex_byte(msg + 12, port & 0xFF);
    memcpy(msg + 14, " (", 2);
    hex_word(msg + 16, port);
    memcpy(msg + 20, ") <- ", 5);
    hex_byte(msg + 25, value);
    msg[27] = '\0';
  } else {
    memcpy(msg + 7, " IN  ", 5);
    hex_byte(msg + 12, port & 0xFF);
    memcpy(msg + 14, " (", 2);
    hex_word(msg + 16, port);
    memcpy(msg + 20, ") -> ", 5);
    hex_byte(msg + 25, value);
    msg[27] = '\0';
  }
  m->host.log(m->host.ctx, msg);
}

uint8_t kaypro_bus_io_read(void *ctx, uint16_t port) {
  kaypro_t *m = (kaypro_t *)ctx;
  uint8_t v = port_bus_read(&m->port_bus, port);
  if (m->trace_io) kaypro_trace_log(m, false, port, v);
  return v;
}

void kaypro_bus_io_write(void *ctx, uint16_t port, uint8_t v) {
  kaypro_t *m = (kaypro_t *)ctx;
  if (m->trace_io) kaypro_trace_log(m, true, port, v);
  /* Baud rate latches (not on FDC). */
  uint8_t p = (uint8_t)(port & 0xFF);
  if (p == 0x00 || p == 0x08) {
    kaypro_sio_set_baud(m->sio, v);
  }
  port_bus_write(&m->port_bus, port, v);
}
