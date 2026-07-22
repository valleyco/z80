#ifndef KAYPRO_FDC1793_H
#define KAYPRO_FDC1793_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"

typedef struct kaypro_fdc {
  EmuDevice emu;
} kaypro_fdc_t;

kaypro_fdc_t *kaypro_fdc_create(void);
void fdc1793_set_sysport(kaypro_fdc_t *fdc, uint8_t sysport);
bool kaypro_fdc_attach(kaypro_fdc_t *fdc, int drive, const char *path);
bool kaypro_fdc_needs_nmi(const kaypro_fdc_t *fdc);
void kaypro_fdc_clear_nmi(kaypro_fdc_t *fdc);

#endif /* KAYPRO_FDC1793_H */
