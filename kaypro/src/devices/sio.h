#ifndef KAYPRO_SIO_H
#define KAYPRO_SIO_H

#include <stdint.h>

#include "device.h"

typedef struct kaypro_sio {
  EmuDevice emu;
} kaypro_sio_t;

kaypro_sio_t *kaypro_sio_create(void);
void kaypro_sio_push_rx_b(kaypro_sio_t *sio, uint8_t byte);
void kaypro_sio_set_baud(kaypro_sio_t *sio, uint8_t code);

#endif /* KAYPRO_SIO_H */
