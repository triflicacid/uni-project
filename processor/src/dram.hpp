#pragma once

#include <cstdint>
#include <array>

namespace processor {
  /** @brief The processor's fixed-size, byte-addressable main memory. */
  class dram {
  public:
    /** @brief Size of the memory in bytes (1 MiB). */
    static constexpr uint64_t size = 1024 * 1024;

  private:
    std::array<uint8_t, size> mem;

  public:
    /** @brief Construct memory, zero-initialised. */
    dram() = default;

    /** @brief Get a raw pointer to the start of memory. @return Pointer to byte 0. */
    uint8_t *data() { return mem.data(); }

    /**
     * @brief Load a word of a given size from memory.
     * @param addr Address to load from.
     * @param bytes Number of bytes to load.
     * @return The loaded word.
     */
    [[nodiscard]] uint64_t load(uint64_t addr, uint8_t bytes) const;

    /**
     * @brief Store a word of a given size to memory.
     * @param addr Address to store to.
     * @param bytes Number of bytes to store.
     * @param value Value to store.
     */
    void store(uint64_t addr, uint8_t bytes, uint64_t value);

    /** @brief Zero every byte of memory. */
    void clear();

    /**
     * @brief Access a single byte.
     * @param index Byte address.
     * @return Reference to the byte.
     */
    uint8_t &operator[](std::size_t index) {
      return mem[index];
    }

    /**
     * @brief Access a single byte.
     * @param index Byte address.
     * @return Reference to the byte.
     */
    const uint8_t &operator[](std::size_t index) const {
      return mem[index];
    }
  };
}
