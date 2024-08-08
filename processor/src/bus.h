#ifndef _BUS_H_
#define _BUS_H_

#include "dram.h"

// a bus is a transfer route to a DRAM block
// create abstraction layer to allow for middleware
typedef struct bus {
  dram_t dram;
} bus_t;

uint64_t bus_load(const bus_t* bus, uint64_t addr, uint8_t size);
void bus_store(bus_t* bus, uint64_t addr, uint8_t size, uint64_t value);

#endif

