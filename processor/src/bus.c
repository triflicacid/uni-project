#include "bus.h"

uint64_t bus_load(const bus_t* bus, uint64_t addr, uint8_t size) {
  return dram_load(&bus->dram, addr, size);
}

void bus_store(bus_t* bus, uint64_t addr, uint8_t size, uint64_t value) {
  dram_store(&bus->dram, addr, size, value);
}
