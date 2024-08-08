#ifndef _DRAM_H_
#define _DRAM_H_

#include <stdint.h>

// size of DRAM block, 1 MiB
#define DRAM_SIZE (1024 * 1024)
#define DRAM_BASE 0x80000000

typedef struct dram {
  uint8_t mem[DRAM_SIZE];
} dram_t;

// load a word of given size from memory
uint64_t dram_load(const dram_t *dram, uint64_t addr, uint8_t size);

// store size bits of fata at addr
void dram_store(dram_t* dram, uint64_t addr, uint8_t size, uint64_t value);

#endif
