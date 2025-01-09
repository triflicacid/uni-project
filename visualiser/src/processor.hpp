#pragma once

#include <set>
#include "../processor/src/cpu.hpp"
#include "named_fstream.hpp"
#include "data.hpp"

namespace visualiser::processor {
  extern ::processor::CPU cpu;
  extern std::unique_ptr<named_fstream> source;
  extern uint64_t initial_pc;
  extern std::unique_ptr<named_fstream> piped_stdout; // if provided, forward processor output to this file.
  extern std::unique_ptr<named_fstream> piped_stdin; // if provided, source input from this rather than the pane

  extern std::set<const PCLine*> breakpoints; // store set breakpoints

  extern uint64_t pc; // value of $pc (should be equal to cpu.read_pc)
  extern const PCLine* pc_line;

  /** Initialise processor from `source`. */
  void init();

  // reset cpu's $pc to initial value, return the $pc's value
  uint64_t restore_pc();

  // update $pc, but only in state:: -- use CPU's $pc
  void update_pc();

  // update $pc -- sets both CPU's actual pc, state::current_pc, and state::pc_entry
  void update_pc(uint64_t val);
}
