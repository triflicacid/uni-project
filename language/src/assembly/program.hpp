#pragma once

#include <deque>
#include "basic_block.hpp"

namespace lang::assembly {
  enum class Position {
    Start,
    Previous = 1,
    Before = 1,
    Next = 2,
    After = 2,
    End,
  };

  // a program is a navigable sequence of basic blocks
  class Program {
    std::deque<std::unique_ptr<BasicBlock>> blocks_;
    std::map<std::string, int> labels_; // map labels to the index of the BasicBlock
    int current_; // 'pointer' to current BasicBlock

    // insert BasicBlock at the given index, update current_
    void insert_at(int index, std::unique_ptr<BasicBlock> block);

  public:
    explicit Program(std::string start_label);

    // return the currently selected block
    BasicBlock& current() { return *blocks_[current_]; }

    // insert the given block into the structure at the given position (relative to current_)
    // sets the inserted block as current_
    void insert(Position pos, std::unique_ptr<BasicBlock> block);

    // select the given block, return success
    bool select(const BasicBlock& block);

    // select the block with the given label
    bool select(const std::string& label);

    // select the block relative to current_
    void select(Position pos);

    std::ostream& print(std::ostream& os) const;
  };
}
