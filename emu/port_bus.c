#include <stdio.h>
#include <stdlib.h>

#include "port_bus.h"

void port_bus_init(PortBus *bus) {
  for (int i = 0; i < 256; i++) {
    bus->dev_read[i] = NULL;
    bus->dev_write[i] = NULL;
  }
}

bool port_bus_register(PortBus *bus, PortDevice *dev, int start_port) {
  dev->port_offset = start_port;
  for (int n = 0; n < dev->port_count; n++) {
    if (dev->read && dev->read[n]) {
      int p = start_port + n;
      if (p < 0 || p > 255) return false;
      if (bus->dev_read[p]) {
        fprintf(stderr, "port read collision on port %02xh\n", p);
        return false;
      }
      bus->dev_read[p] = dev;
    }
    if (dev->write && dev->write[n]) {
      int p = start_port + n;
      if (p < 0 || p > 255) return false;
      if (bus->dev_write[p]) {
        fprintf(stderr, "port write collision on port %02xh\n", p);
        return false;
      }
      bus->dev_write[p] = dev;
    }
  }
  return true;
}

uint8_t port_bus_read(PortBus *bus, uint16_t port) {
  int p = (int)(port & 0xFF);
  PortDevice *dev = bus->dev_read[p];
  if (!dev || !dev->read) return 0;
  int v_port = p - dev->port_offset;
  if (v_port < 0 || v_port >= dev->port_count || !dev->read[v_port]) return 0;
  return (uint8_t)dev->read[v_port](dev, v_port);
}

void port_bus_write(PortBus *bus, uint16_t port, uint8_t value) {
  int p = (int)(port & 0xFF);
  PortDevice *dev = bus->dev_write[p];
  if (!dev || !dev->write) return;
  int v_port = p - dev->port_offset;
  if (v_port < 0 || v_port >= dev->port_count || !dev->write[v_port]) return;
  dev->write[v_port](dev, v_port, value);
}
