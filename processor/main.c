#include "cpu.h"

#include "constants.h"

static cpu_t _cpu;
static cpu_t *cpu = &_cpu;

void print_bin(uint64_t word) {
  uint8_t *ptr = (uint8_t *) &word;

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

  uint8_t reg1 = REG_GPR, reg2 = REG_GPR + 1;

  REG(reg1) = 4;
  REG(reg2) = 2;

  uint64_t data = 0
            | ((uint64_t) reg2 << (OP_HEADER_SIZE + DATATYPE_SIZE + ARG_REG_SIZE + 2))
            | (ARG_REG << (OP_HEADER_SIZE + DATATYPE_SIZE + ARG_REG_SIZE))
            | (reg1 << OP_HEADER_SIZE + DATATYPE_SIZE)
            | (DATATYPE_U64 << OP_HEADER_SIZE)
            | OP_COMPARE;
  MEMWRITE(0, data);

  cpu_start(cpu);
  return 0;
}
