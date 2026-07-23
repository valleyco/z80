#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sio.h"

#define SIO_RX_CAP 256

typedef struct {
  uint8_t buf[SIO_RX_CAP];
  unsigned head;
  unsigned tail;
  unsigned count;
} sio_fifo_t;

typedef struct {
  EmuDevice emu;
  uint8_t baud;
  uint8_t ctrl_a;
  uint8_t ctrl_b;
  sio_fifo_t rx_a;
  sio_fifo_t rx_b;
  int tx_count;
} sio_impl_t;

static void fifo_push(sio_fifo_t *q, uint8_t v) {
  if (q->count >= SIO_RX_CAP) return;
  q->buf[q->head] = v;
  q->head = (q->head + 1) % SIO_RX_CAP;
  q->count++;
}

static bool fifo_pop(sio_fifo_t *q, uint8_t *out) {
  if (q->count == 0) return false;
  *out = q->buf[q->tail];
  q->tail = (q->tail + 1) % SIO_RX_CAP;
  q->count--;
  return true;
}

void kaypro_sio_push_rx_b(kaypro_sio_t *sio, uint8_t byte) {
  fifo_push(&((sio_impl_t *)sio)->rx_b, byte);
}

void kaypro_sio_set_baud(kaypro_sio_t *sio, uint8_t code) {
  ((sio_impl_t *)sio)->baud = code;
}

static int sio_read_a_data(void *dev, int port) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  uint8_t v = 0;
  if (fifo_pop(&sio->rx_a, &v)) return v;
  return 0;
}

static void sio_write_a_data(void *dev, int port, int value) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  (void)value;
  /* Console is the CRT on Universal board; SIO A is the serial port. */
  sio->tx_count++;
}

static int sio_read_a_ctrl(void *dev, int port) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  int status = 0x04; /* Tx buffer empty */
  if (sio->rx_a.count > 0) status |= 0x01;
  return status;
}

static void sio_write_a_ctrl(void *dev, int port, int value) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  sio->ctrl_a = (uint8_t)value;
}

static int sio_read_b_data(void *dev, int port) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  uint8_t v = 0;
  if (fifo_pop(&sio->rx_b, &v)) return v;
  return 0;
}

static void sio_write_b_data(void *dev, int port, int value) {
  (void)dev;
  (void)port;
  (void)value;
}

static int sio_read_b_ctrl(void *dev, int port) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  int status = 0x04; /* Tx buffer empty */
  if (sio->rx_b.count > 0) status |= 0x01;
  return status;
}

static void sio_write_b_ctrl(void *dev, int port, int value) {
  sio_impl_t *sio = (sio_impl_t *)dev;
  (void)port;
  sio->ctrl_b = (uint8_t)value;
}

static void sio_reset(EmuDevice *dev) {
  sio_impl_t *sio = (sio_impl_t *)dev->ctx;
  memset(sio->rx_a.buf, 0, sizeof(sio->rx_a.buf));
  memset(sio->rx_b.buf, 0, sizeof(sio->rx_b.buf));
  sio->rx_a = sio->rx_b = (sio_fifo_t){0};
  sio->ctrl_a = sio->ctrl_b = 0;
  sio->tx_count = 0;
}

static void sio_destroy(EmuDevice *dev) { free(dev->ctx); }

static int (*sio_reads[])(void *, int) = {
    sio_read_a_data, sio_read_b_data, sio_read_a_ctrl, sio_read_b_ctrl};
static void (*sio_writes[])(void *, int, int) = {
    sio_write_a_data, sio_write_b_data, sio_write_a_ctrl, sio_write_b_ctrl};

kaypro_sio_t *kaypro_sio_create(void) {
  sio_impl_t *sio = (sio_impl_t *)calloc(1, sizeof(sio_impl_t));
  if (!sio) return NULL;

  sio->emu.ctx = sio;
  sio->emu.port.port_count = 4;
  sio->emu.port.read = sio_reads;
  sio->emu.port.write = sio_writes;
  sio->emu.reset = sio_reset;
  sio->emu.destroy = sio_destroy;
  return (kaypro_sio_t *)sio;
}
