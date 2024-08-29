#include "bus.h"
#include "debug.h"

#if DEBUG & DEBUG_DRAM
#include "util.h"
#include <stdio.h>
#endif

uint64_t bus_load(const bus_t *bus, uint64_t addr, uint8_t size) {
#if DEBUG & DEBUG_DRAM
  uint64_t value = dram_load(&bus->dram, addr, size);
  printf(DEBUG_STR ANSI_YELLOW " bus load" ANSI_RESET ": %d-bit value from address 0x%llx = 0x%llx\n", size, addr, value);
  return value;
#else
  return dram_load(&bus->dram, addr, size);
#endif
}

void bus_store(bus_t *bus, uint64_t addr, uint8_t size, uint64_t value) {
#if DEBUG & DEBUG_DRAM
  printf(DEBUG_STR ANSI_YELLOW " bus store" ANSI_RESET ": %d-bit value 0x%llx to address 0x%llx\n", size, value,
         addr);
#endif

  dram_store(&bus->dram, addr, size, value);
}
