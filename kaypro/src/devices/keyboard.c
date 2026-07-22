#include <stdlib.h>

#include "keyboard.h"
#include "sio.h"

typedef struct {
  EmuDevice emu;
  kaypro_sio_t *sio;
} keyboard_impl_t;

static void keyboard_poll(EmuDevice *dev) { (void)dev; }

static void keyboard_reset(EmuDevice *dev) { (void)dev; }

static void keyboard_destroy(EmuDevice *dev) { free(dev->ctx); }

kaypro_keyboard_t *kaypro_keyboard_create(kaypro_sio_t *sio) {
  keyboard_impl_t *kbd = (keyboard_impl_t *)calloc(1, sizeof(keyboard_impl_t));
  if (!kbd) return NULL;
  kbd->sio = sio;
  kbd->emu.ctx = kbd;
  kbd->emu.poll = keyboard_poll;
  kbd->emu.reset = keyboard_reset;
  kbd->emu.destroy = keyboard_destroy;
  return (kaypro_keyboard_t *)kbd;
}
