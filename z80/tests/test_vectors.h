#ifndef TEST_VECTORS_H
#define TEST_VECTORS_H

#include <stdint.h>

#define TV_MAGIC "Z80T"
#define TV_VERSION 1

#define TV_MASK_AF (1u << 0)
#define TV_MASK_BC (1u << 1)
#define TV_MASK_DE (1u << 2)
#define TV_MASK_HL (1u << 3)
#define TV_MASK_IX (1u << 4)
#define TV_MASK_IY (1u << 5)
#define TV_MASK_SP (1u << 6)
#define TV_MASK_PC (1u << 7)
#define TV_MASK_AF_ (1u << 8)
#define TV_MASK_BC_ (1u << 9)
#define TV_MASK_DE_ (1u << 10)
#define TV_MASK_HL_ (1u << 11)
#define TV_MASK_I (1u << 12)
#define TV_MASK_R (1u << 13)
#define TV_MASK_IM (1u << 14)
#define TV_MASK_IFF1 (1u << 15)
#define TV_MASK_IFF2 (1u << 16)
#define TV_MASK_HALTED (1u << 17)
#define TV_MASK_FETCH_TRAP (1u << 18)
#define TV_MASK_A (1u << 19)
#define TV_MASK_F (1u << 20)

#define TV_MASK_FLAG_S (1u << 24)
#define TV_MASK_FLAG_Z (1u << 25)
#define TV_MASK_FLAG_Y (1u << 26)
#define TV_MASK_FLAG_X (1u << 27)
#define TV_MASK_FLAG_H (1u << 28)
#define TV_MASK_FLAG_PV (1u << 29)
#define TV_MASK_FLAG_N (1u << 30)
#define TV_MASK_FLAG_C (1u << 31)

/* tv_cpu_state_t.r8_mask — half-register init/expect (b c d e h l) */
#define TV_R8_B (1u << 0)
#define TV_R8_C (1u << 1)
#define TV_R8_D (1u << 2)
#define TV_R8_E (1u << 3)
#define TV_R8_H (1u << 4)
#define TV_R8_L (1u << 5)

#define TV_STATE_SIZE 36
#define TV_MAX_STEPS 100000u

typedef struct {
  uint16_t af, bc, de, hl;
  uint16_t ix, iy, sp, pc;
  uint16_t af_, bc_, de_, hl_;
  uint8_t i, r, im, iff1, iff2;
  uint8_t flags;
  uint8_t r8_mask; /* TV_R8_* for half-register init/expect */
  uint8_t _pad[5];
} tv_cpu_state_t;

typedef struct {
  uint16_t addr;
  uint8_t val;
} tv_mem_io_t;

typedef struct {
  char *name;
  uint16_t start_pc;
  uint16_t end_pc;
  uint32_t code_off;
  uint32_t code_len;
  uint32_t init_mask;
  uint32_t expect_mask;
  tv_cpu_state_t init;
  tv_cpu_state_t expect;
  tv_mem_io_t *mem;
  uint16_t mem_count;
  tv_mem_io_t *io;
  uint16_t io_count;
  tv_mem_io_t *expect_mem;
  uint16_t expect_mem_count;
} tv_test_t;

typedef struct {
  tv_test_t *tests;
  uint16_t count;
  uint8_t *code_blob;
  uint32_t code_blob_size;
} tv_file_t;

int tv_load_file(const char *path, tv_file_t *out);
void tv_free_file(tv_file_t *f);

#endif /* TEST_VECTORS_H */
