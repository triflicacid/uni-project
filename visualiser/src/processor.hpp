#pragma once

#include "../processor/src/cpu.hpp"
#include "named_fstream.hpp"

namespace visualiser::processor {
  extern ::processor::CPU cpu;
  extern std::unique_ptr<named_fstream> source;
  extern uint64_t initial_pc;
  extern std::ostringstream debug_stream;
  extern std::unique_ptr<named_fstream> piped_stdout; // if provided, forward processor output to this file.
  extern std::unique_ptr<named_fstream> piped_stdin; // if provided, source input from this rather than the pane

  /** Initialise processor from `source`. */
  void init();

  // reset cpu's $pc to initial value, return the $pc's value
  uint64_t restore_pc();
}
