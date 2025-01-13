#pragma once

#include <cstdint>

namespace uint64 {
  uint64_t from(uint8_t);
  uint64_t from(int8_t);
  uint64_t from(uint16_t);
  uint64_t from(int16_t);
  uint64_t from(uint32_t);
  uint64_t from(int32_t);
  uint64_t from(uint64_t);
  uint64_t from(int64_t);
  uint64_t from(float);
  uint64_t from(double);

  double to_double(uint64_t);
  float to_float(uint64_t);
  int8_t to_int8(uint64_t);
  uint8_t to_uint8(uint64_t);
  int16_t to_int16(uint64_t);
  uint16_t to_uint16(uint64_t);
  int32_t to_int32(uint64_t);
  uint32_t to_uint32(uint64_t);
  int64_t to_int64(uint64_t);
  uint64_t to_uint64(uint64_t);
}
