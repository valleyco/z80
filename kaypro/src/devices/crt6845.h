#ifndef KAYPRO_CRT6845_H
#define KAYPRO_CRT6845_H

#include "device.h"

typedef struct kaypro_crt {
  EmuDevice emu;
} kaypro_crt_t;

/* SY6545-compatible ports at 1Ch-1Fh: status must report ready (bit7). */
kaypro_crt_t *kaypro_crt_create(void);

#endif /* KAYPRO_CRT6845_H */
