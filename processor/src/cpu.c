#include "cpu.h"

#include <string.h>

void cpu_init(cpu_t *cpu) {
  // set default file I/O handlers
  cpu->fp_out = stdout;
  cpu->fp_in = stdin;

  // clear and configure key registers
  memset(cpu->regs, 0, sizeof(cpu->regs));
  cpu->regs[REG_SP] = DRAM_SIZE;
  cpu->regs[REG_FP] = cpu->regs[REG_SP];

#if DEBUG & DEBUG_CPU
  printf("Initialising CPU... Done.\n");
#endif
}
