#ifndef KAYPRO_HDC_WD1002_H
#define KAYPRO_HDC_WD1002_H

#include "device.h"

typedef struct kaypro_hdc {
  EmuDevice emu;
} kaypro_hdc_t;

/* WD1002-HD0 task file at 80h-87h. Absent-controller stub fails winrest fast. */
kaypro_hdc_t *kaypro_hdc_create(void);

#endif /* KAYPRO_HDC_WD1002_H */
