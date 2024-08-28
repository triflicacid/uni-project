#include <constants.h>
#include <stdio.h>
#include <stdint.h>

int main() {
  uint64_t data = 0x1fcccccd00c0c3e0;
  printf("Data: 0x%llx\n", data);

  uint8_t datatype = (data >> OP_HEADER_SIZE) & 0x7;
  printf("Data Type: 0x%x\n", datatype);

  return 0;
}
