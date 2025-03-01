#pragma once

#include <deque>
#include <map>
#include <stack>
#include "basic_block.hpp"
#include "optional_ref.hpp"

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
    std::string start_label;
    std::deque<std::unique_ptr<BasicBlock>> blocks_;
    std::map<std::string, std::reference_wrapper<BasicBlock>> labels_; // map labels to the index of the BasicBlock
    int current_; // 'pointer' to current BasicBlock
    std::stack<Location> locations_; // track source locations

    // insert BasicBlock at the given index, update current_
    void insert_at(int index, std::unique_ptr<BasicBlock> block);

  public:
    explicit Program(std::string start_label);

    // return the currently selected block
    BasicBlock& current() const { return *blocks_[current_]; }

    // return reference to the given block, errors if fail
    BasicBlock& get(const std::string& label);

    // insert the given block into the structure at the given position (relative to current_)
    // sets the inserted block as current_
    void insert(Position pos, std::unique_ptr<BasicBlock> block);

    // select the given block, return success
    bool select(const BasicBlock& block);

    // select the block with the given label
    bool select(const std::string& label);

    // select the block relative to current_
    void select(Position pos);

    // add a Location to the trace stack
    void add_location(Location loc);

    // get the most recent location
    optional_ref<const Location> location() const;

    // remove the topmost Location
    void remove_location();

    // set the topmost location to the given value
    void set_location(Location loc);

    // set current line's location (or from given index to most recent)
    // only update's a line's origin if it has not already been set (unless sudo is true)
    void update_line_origins(const Location& origin, int start = -1, bool sudo = false) const;

    // same as ::update_line_origins, but use current location
    void update_line_origins(int start = -1, bool sudo = false) const;

    std::ostream& print(std::ostream& os) const;
  };
}
