#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt6845.h"

typedef struct {
  EmuDevice emu;
  uint8_t reg_sel;
  uint8_t regs[32];
  uint16_t mem_addr;
  bool attr_plane;
  bool last_graph;
} crt_impl_t;

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
    putchar('\n');
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x0A) {
    putchar('\n');
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x08) {
    fputs("\b \b", stdout);
    fflush(stdout);
    crt->last_graph = false;
    return;
  }
  if (c == 0x07) return;

  if (c == 0x20) {
    /* Suppress clear-screen space floods; keep inter-word spaces. */
    if (crt->last_graph) {
      putchar(' ');
      fflush(stdout);
      crt->last_graph = false;
    }
    return;
  }

  if (c >= 0x21 && c < 0x7F) {
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
}

static void crt_destroy(EmuDevice *dev) { free(dev->ctx); }

static int (*crt_reads[])(void *, int) = {
    crt_read_status, crt_read_regdata, crt_read_unused, crt_read_data};
static void (*crt_writes[])(void *, int, int) = {
    crt_write_cmd, crt_write_regdata, crt_write_unused, crt_write_data};

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
