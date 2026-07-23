#ifndef KAYPRO_INTERNAL_H
#define KAYPRO_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "port_bus.h"
#include "z80.h"

#define KAYPRO_ROM_SIZE 8192

typedef struct kaypro_sysport kaypro_sysport_t;
typedef struct kaypro_sio kaypro_sio_t;
typedef struct kaypro_fdc kaypro_fdc_t;
typedef struct kaypro_keyboard kaypro_keyboard_t;
typedef struct kaypro_crt kaypro_crt_t;
typedef struct kaypro_hdc kaypro_hdc_t;

typedef struct kaypro {
  z80_t cpu;
  uint8_t ram[65536];
  uint8_t rom[KAYPRO_ROM_SIZE];
  bool bank1;
  PortBus port_bus;
  EmuDevice **devices;
  int device_count;
  int device_cap;
  bool needs_nmi;
  bool trace_io;

  kaypro_sysport_t *sysport;
  kaypro_sio_t *sio;
  kaypro_fdc_t *fdc;
  kaypro_keyboard_t *keyboard;
  kaypro_crt_t *crt;
  kaypro_hdc_t *hdc;
} kaypro_t;

void kaypro_mem_init(kaypro_t *m);
uint8_t kaypro_bus_mem_read(void *ctx, uint16_t addr);
void kaypro_bus_mem_write(void *ctx, uint16_t addr, uint8_t v);
uint8_t kaypro_bus_io_read(void *ctx, uint16_t port);
void kaypro_bus_io_write(void *ctx, uint16_t port, uint8_t v);

void kaypro_add_device(kaypro_t *m, EmuDevice *dev);
void kaypro_register_ports(kaypro_t *m);

#endif /* KAYPRO_INTERNAL_H */
