#pragma once

#include <cstdint>
#include <deque>
#include <stack>
#include "assembly/program.hpp"

namespace lang::memory {
  // class to manage storing and retrieving values from the stack
  class StackManager {
    uint64_t offset_ = 0; // record offset into the stack
    std::deque<uint64_t> frames_; // cached offset_'s of previous stack frames, most recent = front
    assembly::Program& program_;

  public:
    StackManager(assembly::Program& program) : program_(program) {}

    assembly::Program& program() { return program_; }

    uint64_t offset() const { return offset_; }

    // increase offset by the given byte count
    void push(uint8_t bytes);

    // decrease offset by the given byte count
    void pop(uint8_t bytes);

    // create a new stack frame
    void push_frame();

    // remove latest stack frame
    void pop_frame();

    // get the address of the nth topmost frame (default n = 0 = most recent)
    uint64_t peek_frame(unsigned int n = 0) const;
  };
}
