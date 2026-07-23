#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt6845.h"

#define CRT_TX_CAP 256

typedef struct {
  EmuDevice emu;
  uint8_t reg_sel;
  uint8_t regs[32];
  uint16_t mem_addr;
  bool attr_plane;
  bool last_graph;
  uint8_t tx[CRT_TX_CAP];
  unsigned tx_count;
} crt_impl_t;

static void crt_tx_push(crt_impl_t *crt, uint8_t c) {
  if (crt->tx_count < CRT_TX_CAP) crt->tx[crt->tx_count++] = c;
}

static int crt_read_status(void *dev, int port) {
  (void)dev;
  (void)port;
  /* Bit 7 set = update ready (ROM waits with `or a; jp p,...`). */
  return 0x80;
}

static int crt_read_regdata(void *dev, int port) {
  crt_impl_t *crt = (crt_impl_t *)dev;
  (void)port;
  if (crt->reg_sel < 32) return crt->regs[crt->reg_sel];
  return 0;
}

static int crt_read_unused(void *dev, int port) {
  (void)dev;
  (void)port;
  return 0;
}

static int crt_read_data(void *dev, int port) {
  (void)dev;
  (void)port;
  return 0x20;
}

static void crt_write_cmd(void *dev, int port, int value) {
  crt_impl_t *crt = (crt_impl_t *)dev;
  (void)port;
  crt->reg_sel = (uint8_t)(value & 0x1F);
}

static void crt_apply_addr(crt_impl_t *crt) {
  uint16_t hi = crt->regs[0x12];
  uint16_t lo = crt->regs[0x13];
  crt->attr_plane = (hi & 0x08) != 0;
  crt->mem_addr = (uint16_t)(((hi & 0x07) << 8) | lo);
}

static void crt_write_regdata(void *dev, int port, int value) {
  crt_impl_t *crt = (crt_impl_t *)dev;
  (void)port;
  if (crt->reg_sel < 32) crt->regs[crt->reg_sel] = (uint8_t)value;
  if (crt->reg_sel == 0x12 || crt->reg_sel == 0x13) crt_apply_addr(crt);
}

static void crt_write_unused(void *dev, int port, int value) {
  (void)dev;
  (void)port;
  (void)value;
}

static void crt_echo(crt_impl_t *crt, uint8_t c) {
  if (crt->attr_plane) return;

  if (c == 0x0D) {
    crt_tx_push(crt, '\n');
    putchar('\n');
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x0A) {
    crt_tx_push(crt, '\n');
    putchar('\n');
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x08) {
    crt_tx_push(crt, 0x08);
    fputs("\b \b", stdout);
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x07) return;

  if (c == 0x20) {
    /* Suppress clear-screen space floods; keep inter-word spaces. */
    if (crt->last_graph) {
      crt_tx_push(crt, ' ');
      putchar(' ');
      fflush(stdout);
      crt->last_graph = false;
    }
    return;
  }

  if (c >= 0x21 && c < 0x7F) {
    crt_tx_push(crt, c);
    putchar((char)c);
    fflush(stdout);
    crt->last_graph = true;
  }
}

static void crt_write_data(void *dev, int port, int value) {
  crt_impl_t *crt = (crt_impl_t *)dev;
  (void)port;
  crt_echo(crt, (uint8_t)value);
  crt->mem_addr = (uint16_t)((crt->mem_addr + 1) & 0x7FF);
}

static void crt_reset(EmuDevice *dev) {
  crt_impl_t *crt = (crt_impl_t *)dev->ctx;
  memset(crt->regs, 0, sizeof(crt->regs));
  crt->reg_sel = 0;
  crt->mem_addr = 0;
  crt->attr_plane = false;
  crt->last_graph = false;
  crt->tx_count = 0;
}

static void crt_destroy(EmuDevice *dev) { free(dev->ctx); }

static int (*crt_reads[])(void *, int) = {
    crt_read_status, crt_read_regdata, crt_read_unused, crt_read_data};
static void (*crt_writes[])(void *, int, int) = {
    crt_write_cmd, crt_write_regdata, crt_write_unused, crt_write_data};

unsigned kaypro_crt_tx_count(const kaypro_crt_t *crt) {
  return ((const crt_impl_t *)crt)->tx_count;
}

uint8_t kaypro_crt_tx_at(const kaypro_crt_t *crt, unsigned index) {
  const crt_impl_t *impl = (const crt_impl_t *)crt;
  if (index >= impl->tx_count) return 0;
  return impl->tx[index];
}

void kaypro_crt_tx_clear(kaypro_crt_t *crt) {
  ((crt_impl_t *)crt)->tx_count = 0;
}

kaypro_crt_t *kaypro_crt_create(void) {
  crt_impl_t *crt = (crt_impl_t *)calloc(1, sizeof(crt_impl_t));
  if (!crt) return NULL;

  crt->emu.ctx = crt;
  crt->emu.port.port_count = 4;
  crt->emu.port.read = crt_reads;
  crt->emu.port.write = crt_writes;
  crt->emu.reset = crt_reset;
  crt->emu.destroy = crt_destroy;
  return (kaypro_crt_t *)crt;
}
