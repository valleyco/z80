#ifndef KAYPRO_CRT6845_H
#define KAYPRO_CRT6845_H

#include <stdint.h>

#include "device.h"

typedef struct kaypro_crt {
  EmuDevice emu;
} kaypro_crt_t;

/* SY6545-compatible ports at 1Ch-1Fh: status must report ready (bit7). */
kaypro_crt_t *kaypro_crt_create(void);

/* Captured console echo bytes (char plane only), for smoke tests. */
unsigned kaypro_crt_tx_count(const kaypro_crt_t *crt);
uint8_t kaypro_crt_tx_at(const kaypro_crt_t *crt, unsigned index);
void kaypro_crt_tx_clear(kaypro_crt_t *crt);

#endif /* KAYPRO_CRT6845_H */
