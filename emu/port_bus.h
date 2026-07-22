#ifndef EMU_PORT_BUS_H
#define EMU_PORT_BUS_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  PortDevice *dev_read[256];
  PortDevice *dev_write[256];
} PortBus;

void port_bus_init(PortBus *bus);
bool port_bus_register(PortBus *bus, PortDevice *dev, int start_port);
uint8_t port_bus_read(PortBus *bus, uint16_t port);
void port_bus_write(PortBus *bus, uint16_t port, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* EMU_PORT_BUS_H */
