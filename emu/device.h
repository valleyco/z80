#ifndef EMU_DEVICE_H
#define EMU_DEVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*emu_port_read_fn)(void *dev, int port);
typedef void (*emu_port_write_fn)(void *dev, int port, int value);

typedef struct {
  int port_count;
  int port_offset;
  emu_port_read_fn *read;
  emu_port_write_fn *write;
} PortDevice;

typedef struct EmuDevice EmuDevice;

struct EmuDevice {
  PortDevice port;
  void *ctx;
  void (*reset)(EmuDevice *dev);
  void (*poll)(EmuDevice *dev);
  void (*destroy)(EmuDevice *dev);
};

#ifdef __cplusplus
}
#endif

#endif /* EMU_DEVICE_H */
