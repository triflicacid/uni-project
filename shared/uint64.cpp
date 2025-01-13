#include "uint64.hpp"

uint64_t uint64::from(uint8_t x) {
  return x;
}

uint64_t uint64::from(int8_t x) {
  return *(uint8_t*)&x;
}

uint64_t uint64::from(uint16_t x) {
  return x;
}

uint64_t uint64::from(int16_t x) {
  return *(uint16_t*)&x;
}

uint64_t uint64::from(uint32_t x) {
  return x;
}

uint64_t uint64::from(int32_t x) {
  return *(uint32_t*)&x;
}

uint64_t uint64::from(uint64_t x) {
  return x;
}

uint64_t uint64::from(int64_t x) {
  return *(uint16_t*)&x;
}

uint64_t uint64::from(float x) {
  return *(uint32_t*)&x;
}

uint64_t uint64::from(double x) {
  return *(uint64_t*)&x;
}

double uint64::to_double(uint64_t x) {
  return *(double*)&x;
}

float uint64::to_float(uint64_t x) {
  uint32_t y = *(uint32_t*)&x;
  return *(float*)&y;
}

int8_t uint64::to_int8(uint64_t x) {
  uint8_t y = *(uint8_t*)&x;
  return *(int8_t*)&y;
}

uint8_t uint64::to_uint8(uint64_t x) {
  return *(uint8_t*)&x;
}

int16_t uint64::to_int16(uint64_t x) {
  uint16_t y = *(uint16_t*)&x;
  return *(int16_t*)&y;
}

uint16_t uint64::to_uint16(uint64_t x) {
  return *(uint16_t*)&x;
}

int32_t uint64::to_int32(uint64_t x) {
  uint32_t y = *(uint32_t*)&x;
  return *(int32_t*)&y;
}

uint32_t uint64::to_uint32(uint64_t x) {
  return *(uint32_t*)&x;
}

int64_t uint64::to_int64(uint64_t x) {
  return *(int64_t*)&x;
}

uint64_t uint64::to_uint64(uint64_t x) {
  return x;
}
