#include "bus.hpp"
#include "debug.hpp"

void processor::bus::store(uint64_t addr, uint8_t size, uint64_t bytes) {
    mem.store(addr, size, bytes);
}

uint64_t processor::bus::load(uint64_t addr, uint8_t size) const {
    return mem.load(addr, size);
}
