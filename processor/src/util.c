#include "util.h"

char *cmp_bit_str(uint8_t bits) {
  switch (bits) {
    case 0b000: return "ne";
    case 0b001: return "eq";
    case 0b010: return "lt";
    case 0b011: return "le";
    case 0b100: return "z";
    case 0b110: return "gt";
    case 0b111: return "ge";
    default: return "?";
  }
}
