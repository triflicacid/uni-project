#define DEBUG 0xFF

#include "cpu.h"

static cpu_t cpu;

int main() {
  cpu_init(&cpu);

  printf("Hello, world!\n");
  return 0;
}
