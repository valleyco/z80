#ifndef KAYPRO_KEYBOARD_H
#define KAYPRO_KEYBOARD_H

#include "device.h"
#include "sio.h"

typedef struct kaypro_keyboard {
  EmuDevice emu;
} kaypro_keyboard_t;

kaypro_keyboard_t *kaypro_keyboard_create(kaypro_sio_t *sio);

#endif /* KAYPRO_KEYBOARD_H */
