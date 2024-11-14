#include "bus.hpp"
#include "debug.hpp"

#if DEBUG & DEBUG_DRAM
#include "util.h"
#include <stdio.h>
#endif

void processor::bus::store(uint64_t addr, uint8_t size, uint64_t value) {
#if DEBUG & DEBUG_DRAM
    printf(DEBUG_STR ANSI_YELLOW " bus store" ANSI_RESET ": %d-bit value 0x%llx to address 0x%llx\n", size, value,
           addr);
#endif

    mem.store(addr, size, value);
}

uint64_t processor::bus::load(uint64_t addr, uint8_t size) const {
#if DEBUG & DEBUG_DRAM
    uint64_t value = dram_load(&bus->dram, addr, size);
    printf(DEBUG_STR ANSI_YELLOW " bus load" ANSI_RESET ": %d-bit value from address 0x%llx = 0x%llx\n", size, addr, value);
    return value;
#else
    return mem.load(addr, size);
#endif
}
