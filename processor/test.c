#include <constants.h>
#include <cpu.h>
#include <stdio.h>
#include <stdint.h>

cpu_t cpu;

int main() {
  cpu_init(&cpu);

  printf("$sp = 0x%llx\n", cpu.regs[REG_SP]);
  printf("$sp - 8 = 0x%llx\n", cpu.regs[REG_SP] - 8);

  return 0;
}
