#ifndef KAYPRO_SYSPORT_H
#define KAYPRO_SYSPORT_H

#include "device.h"

typedef struct kaypro kaypro_t;
typedef struct kaypro_fdc kaypro_fdc_t;

typedef struct kaypro_sysport {
  EmuDevice emu;
} kaypro_sysport_t;

kaypro_sysport_t *kaypro_sysport_create(kaypro_t *machine, kaypro_fdc_t *fdc);
void kaypro_sysport_sync_bank(kaypro_sysport_t *sys);
uint8_t kaypro_sysport_latch(const kaypro_sysport_t *sys);
void kaypro_sysport_set_latch(kaypro_sysport_t *sys, uint8_t latch);

#endif /* KAYPRO_SYSPORT_H */
