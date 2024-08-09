#include "dram.h"

#include <string.h>

static inline uint64_t dram_load_8(const dram_t *dram, uint64_t addr) {
  return dram->mem[addr];
}

static inline uint64_t dram_load_16(const dram_t *dram, uint64_t addr) {
  return (uint64_t) dram->mem[addr]
      |  (uint64_t) dram->mem[addr + 1] << 8;
}

static inline uint64_t dram_load_32(const dram_t *dram, uint64_t addr) {
  return (uint64_t) dram->mem[addr]
      |  (uint64_t) dram->mem[addr + 1] << 8
      |  (uint64_t) dram->mem[addr + 2] << 16
      |  (uint64_t) dram->mem[addr + 3] << 24;
}

static inline uint64_t dram_load_64(const dram_t *dram, uint64_t addr) {
  return (uint64_t) dram->mem[addr]
      |  (uint64_t) dram->mem[addr + 1] << 8
      |  (uint64_t) dram->mem[addr + 2] << 16
      |  (uint64_t) dram->mem[addr + 3] << 24
      |  (uint64_t) dram->mem[addr + 4] << 32
      |  (uint64_t) dram->mem[addr + 5] << 40
      |  (uint64_t) dram->mem[addr + 6] << 48
      |  (uint64_t) dram->mem[addr + 7] << 56;
}

uint64_t dram_load(const dram_t *dram, uint64_t addr, uint8_t size) {
  switch (size) {
    case 8:  return dram_load_8(dram, addr);
    case 16: return dram_load_16(dram, addr);
    case 32: return dram_load_32(dram, addr);
    case 64: return dram_load_64(dram, addr);
    default: return 0;
  }
}

static inline void dram_store_8(dram_t *dram, uint64_t addr, uint64_t value) {
  dram->mem[addr] = (uint8_t) (value & 0xff);
}

static inline void dram_store_16(dram_t *dram, uint64_t addr, uint64_t value) {
  dram->mem[addr] = (uint8_t) (value & 0xff);
  dram->mem[addr + 1] = (uint8_t) (value >> 8 & 0xff);
}

static inline void dram_store_32(dram_t *dram, uint64_t addr, uint64_t value) {
  dram->mem[addr] = (uint8_t) (value & 0xff);
  dram->mem[addr + 1] = (uint8_t) (value >> 8  & 0xff);
  dram->mem[addr + 2] = (uint8_t) (value >> 16 & 0xff);
  dram->mem[addr + 3] = (uint8_t) (value >> 24 & 0xff);
}

static inline void dram_store_64(dram_t *dram, uint64_t addr, uint64_t value) {
  dram->mem[addr] = (uint8_t) (value & 0xff);
  dram->mem[addr + 1] = (uint8_t) (value >> 8  & 0xff);
  dram->mem[addr + 2] = (uint8_t) (value >> 16 & 0xff);
  dram->mem[addr + 3] = (uint8_t) (value >> 24 & 0xff);
  dram->mem[addr + 4] = (uint8_t) (value >> 32 & 0xff);
  dram->mem[addr + 5] = (uint8_t) (value >> 40 & 0xff);
  dram->mem[addr + 6] = (uint8_t) (value >> 48 & 0xff);
  dram->mem[addr + 7] = (uint8_t) (value >> 56 & 0xff);
}

void dram_store(dram_t *dram, uint64_t addr, uint8_t size, uint64_t value) {
  switch (size) {
    case 8:  dram_store_8(dram, addr, value);  break;
    case 16: dram_store_16(dram, addr, value); break;
    case 32: dram_store_32(dram, addr, value); break;
    case 64: dram_store_64(dram, addr, value); break;
    default: ;
  }
}

void dram_clear(dram_t *dram) {
  memset(dram->mem, 0, DRAM_SIZE);
}
