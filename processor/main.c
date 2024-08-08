#include "cpu.h"

static cpu_t _cpu;
static cpu_t *cpu = &_cpu;

int main() {
  cpu_init(cpu);

  MEMWRITE(1, 0xf);
  cpu_cycle(cpu);

  return 0;
}
