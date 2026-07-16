#pragma once

#include <set>
#include "../processor/src/cpu.hpp"
#include "named_fstream.hpp"
#include "sources.hpp"

/** @brief The CPU instance and loaded-program state backing the visualiser session. */
namespace visualiser::processor {
  /** The emulated CPU instance backing the visualiser session. */
  extern ::processor::CPU cpu;
  /** Binary file the CPU's program was loaded from. */
  extern std::unique_ptr<named_fstream> source;
  /** Value of `$pc` at load time, used to restore the CPU to its starting state. */
  extern uint64_t initial_pc;
  /** If set, processor output is forwarded to this file instead of the UI pane. */
  extern std::unique_ptr<named_fstream> piped_stdout;
  /** If set, processor input is sourced from this file instead of the UI pane. */
  extern std::unique_ptr<named_fstream> piped_stdin;

  /** Set of source lines with a breakpoint currently set. */
  extern std::set<const sources::PCLine*> breakpoints;

  /** Cached value of `$pc`, kept in sync with `cpu.read_pc()`. */
  extern uint64_t pc;
  /** Source line corresponding to the current value of `pc`, if known. */
  extern const sources::PCLine* pc_line;

  /** @brief Reset the CPU and load the program from `source`. */
  void init();

  /**
   * @brief Reset the CPU's `$pc` to its initial value.
   * @return The restored `$pc` value.
   */
  uint64_t restore_pc();

  /** @brief Refresh the cached `pc` and `pc_line` from the CPU's current `$pc`, without changing the CPU's state. */
  void update_pc();

  /**
   * @brief Set the CPU's `$pc` and refresh the cached `pc` and `pc_line` to match.
   * @param val New program counter value.
   */
  void update_pc(uint64_t val);
}
