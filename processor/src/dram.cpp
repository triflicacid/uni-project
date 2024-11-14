#include <cstring>
#include "dram.hpp"

uint64_t processor::dram::load(uint64_t addr, uint8_t bytes) const {
    uint64_t data = 0;

    for (int i = 0, sh = 0; i < bytes; i++, sh += 8) {
        data |= mem[addr + i] << sh;
    }

    return data;
}

void processor::dram::store(uint64_t addr, uint8_t bytes, uint64_t value) {
    for (int i = 0, sh = 0; i < bytes; i++, sh += 8) {
        mem[addr + i] = (uint8_t) (value >> sh & 0xff);
    }
}

void processor::dram::clear() {
    memset(mem.data(), 0, mem.size());
}
