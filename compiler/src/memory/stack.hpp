#pragma once

#include <cstdint>
#include <deque>
#include <stack>
#include "assembly/program.hpp"

namespace lang::memory {
  /**
   * @brief Tracks the compiled program's runtime stack layout at compile time: the current offset from $fp and a history of saved frame offsets.
   */
  class StackManager {
    uint64_t offset_ = 0; ///< Current offset into the stack from $fp.
    std::deque<uint64_t> frames_; ///< Saved `offset_` values of previous stack frames; front is the most recent.
    assembly::Program& program_; ///< Output program stack-adjustment instructions are emitted into.

  public:
    /**
     * @brief Binds the manager to the program it will emit stack-adjustment instructions into.
     * @param program Output program to emit instructions into.
     */
    StackManager(assembly::Program& program) : program_(program) {}

    /**
     * @brief Returns the underlying program.
     * @return The program.
     */
    assembly::Program& program() { return program_; }

    /**
     * @brief Returns the current stack offset from $fp.
     * @return The offset in bytes.
     */
    uint64_t offset() const { return offset_; }

    /**
     * @brief Reserves additional stack space, updating the compile-time offset and emitting a push instruction.
     * @param bytes Number of bytes to reserve. No-ops if zero.
     */
    void push(uint8_t bytes);

    /**
     * @brief Releases stack space, updating the compile-time offset and emitting a pop instruction.
     * @param bytes Number of bytes to release. No-ops if zero.
     */
    void pop(uint8_t bytes);

    /**
     * @brief Begins a new stack frame, saving the current offset and optionally emitting the $fp-establishing instruction.
     * @param generate_code Whether to emit the instruction copying $sp into $fp.
     */
    void push_frame(bool generate_code);

    /**
     * @brief Ends the current stack frame, restoring the enclosing frame's offset and optionally emitting the $fp-restoring instruction.
     * @param generate_code Whether to emit the instruction restoring $sp from $fp.
     */
    void pop_frame(bool generate_code);

    /**
     * @brief Returns the saved offset of the nth most recently pushed (not yet popped) frame.
     * @param n Depth to look back, where 0 is the most recently pushed frame.
     * @return The saved offset.
     */
    uint64_t peek_frame(unsigned int n = 0) const;
  };
}
