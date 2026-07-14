#pragma once

#include <cstdint>

/**
 * @brief Conversions between register-sized `uint64_t` bit patterns and the concrete datatypes a register can hold.
 *
 * A processor register is a raw 64-bit word; these functions reinterpret its bits as (or pack a
 * smaller value's bits into) a `uint64_t`, matching the datatype tag encoded in an instruction.
 */
namespace uint64 {
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(uint8_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(int8_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(uint16_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(int16_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(uint32_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(int32_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v` unchanged. */
  uint64_t from(uint64_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(int64_t v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(float v);
  /** @brief Pack a value's bits into a `uint64_t`. @param v Value to pack. @return `v`'s bit pattern as a `uint64_t`. */
  uint64_t from(double v);

  /** @brief Reinterpret a `uint64_t`'s bits as a `double`. @param v Bit pattern to reinterpret. @return The resulting `double`. */
  double to_double(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as a `float`. @param v Bit pattern to reinterpret. @return The resulting `float`. */
  float to_float(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as an `int8_t`. @param v Bit pattern to reinterpret. @return The resulting `int8_t`. */
  int8_t to_int8(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as a `uint8_t`. @param v Bit pattern to reinterpret. @return The resulting `uint8_t`. */
  uint8_t to_uint8(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as an `int16_t`. @param v Bit pattern to reinterpret. @return The resulting `int16_t`. */
  int16_t to_int16(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as a `uint16_t`. @param v Bit pattern to reinterpret. @return The resulting `uint16_t`. */
  uint16_t to_uint16(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as an `int32_t`. @param v Bit pattern to reinterpret. @return The resulting `int32_t`. */
  int32_t to_int32(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as a `uint32_t`. @param v Bit pattern to reinterpret. @return The resulting `uint32_t`. */
  uint32_t to_uint32(uint64_t v);
  /** @brief Reinterpret a `uint64_t`'s bits as an `int64_t`. @param v Bit pattern to reinterpret. @return The resulting `int64_t`. */
  int64_t to_int64(uint64_t v);
  /** @brief Return a `uint64_t` unchanged. @param v Value to return. @return `v`. */
  uint64_t to_uint64(uint64_t v);
}
