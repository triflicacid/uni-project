#pragma once

#include "dram.hpp"

namespace processor {
  /**
   * @brief A transfer route between the CPU and a block of DRAM.
   *
   * Exists as a separate seam from @ref dram so middleware (e.g. memory-mapped devices) could be
   * inserted between the CPU and memory in future without changing the CPU's interface.
   */
  struct bus {
    dram mem;

    /**
     * @brief Load a word from memory.
     * @param addr Address to load from.
     * @param size Number of bytes to load.
     * @return The loaded word.
     */
    [[nodiscard]] uint64_t load(uint64_t addr, uint8_t size) const;

    /**
     * @brief Store a word to memory.
     * @param addr Address to store to.
     * @param size Number of bytes to store.
     * @param bytes Value to store.
     */
    void store(uint64_t addr, uint8_t size, uint64_t bytes);
  };
}
