#include "cpu.h"

#include "constants.h"

static cpu_t _cpu;
static cpu_t *cpu = &_cpu;

void print_bin(uint64_t word) {
  uint8_t *ptr = &word;

  for (int i = 0, j = 0; i < 64; i++) {
    printf("%i", *ptr & (1 << j) ? 1 : 0);

    if (j == 7) {
      j = 0;
      ptr++;
    } else {
      j++;
    }
  }

  printf("\n");
}

int main() {
  cpu_init(cpu);

  uint8_t reg = REG_GPR;
  uint64_t data = 0
            | ((uint64_t) 0 << (OP_HEADER_SIZE + ARG_REG_SIZE + 2))
            | (ARG_IMM << (OP_HEADER_SIZE + ARG_REG_SIZE))
            | (reg << OP_HEADER_SIZE)
            | OP_LOAD;
  MEMWRITE(0, data);

  data = 0
            | ((uint64_t) 0xffffffff << (OP_HEADER_SIZE + ARG_REG_SIZE + 2))
            | (ARG_IMM << (OP_HEADER_SIZE + ARG_REG_SIZE))
            | (reg << OP_HEADER_SIZE)
            | OP_LOAD_UPPER;
  MEMWRITE(1, data);

  cpu_cycle(cpu);

  printf("Reg %i: ", reg);
  print_bin(REG(reg));

  return 0;
}
