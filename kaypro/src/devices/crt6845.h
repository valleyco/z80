#ifndef KAYPRO_CRT6845_H
#define KAYPRO_CRT6845_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"

/* Kaypro Universal text mode: 80x25 = 2000 cells in 2K VRAM. */
#define KAYPRO_CRT_COLS 80
#define KAYPRO_CRT_ROWS 25
#define KAYPRO_CRT_CELLS (KAYPRO_CRT_COLS * KAYPRO_CRT_ROWS)

typedef struct kaypro_crt {
  EmuDevice emu;
} kaypro_crt_t;

/* SY6545-compatible ports at 1Ch-1Fh: status must report ready (bit7). */
kaypro_crt_t *kaypro_crt_create(void);

/* Portable character cell buffer (row-major). */
const uint8_t *kaypro_crt_cells(const kaypro_crt_t *crt);
unsigned kaypro_crt_cols(void);
unsigned kaypro_crt_rows(void);
/* Visible cursor from CRT R14/R15 (0-based col/row within the text grid). */
void kaypro_crt_cursor(const kaypro_crt_t *crt, unsigned *col, unsigned *row);
bool kaypro_crt_is_dirty(const kaypro_crt_t *crt);
void kaypro_crt_clear_dirty(kaypro_crt_t *crt);

#endif /* KAYPRO_CRT6845_H */
