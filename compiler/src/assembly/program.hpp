#pragma once

#include <deque>
#include <map>
#include <stack>
#include "basic_block.hpp"
#include "optional_ref.hpp"

/** @brief Generated-assembly intermediate representation: programs, basic blocks, instructions, and directives emitted by code generation. */
namespace lang::assembly {
  /**
   * @brief Position, relative to the current block, used both for inserting new blocks and for moving the navigation cursor.
   *
   * Before/Previous share value 1 and After/Next share value 2 deliberately:
   * insert() uses the positional names, select() uses the navigational ones,
   * but the two are freely interchangeable at the type level.
   */
  enum class Position {
    Start, ///< Before the first block.
    Previous = 1, ///< The block before the cursor (navigational name for value 1).
    Before = 1, ///< Insert before the reference block (positional name for value 1).
    Next = 2, ///< The block after the cursor (navigational name for value 2).
    After = 2, ///< Insert after the reference block (positional name for value 2).
    End, ///< After the last block.
  };

  /**
   * @brief The generated assembly program: an ordered, navigable sequence of basic blocks with a movable insertion cursor, a label lookup map, and a source-location stack.
   */
  class Program {
    std::string start_label; ///< Label of the entry-point block.
    std::deque<std::unique_ptr<BasicBlock>> blocks_; ///< Every block in the program, in order.
    std::map<std::string, std::reference_wrapper<BasicBlock>> labels_; ///< Maps a block's label to the block itself.
    int current_; ///< Index into `blocks_` of the block the cursor currently points at.
    std::stack<Location> locations_; ///< Source-location context stack, tracking nested locations for line origin back-filling.

    /**
     * @brief Physically inserts a block at an absolute index, and repositions the cursor and label map accordingly.
     * @param index Absolute index to insert at.
     * @param block Block to insert.
     */
    void insert_at(int index, std::unique_ptr<BasicBlock> block);

  public:
    /**
     * @brief Constructs a program containing exactly one initial block, the entry point.
     * @param start_label Label of the entry-point block.
     */
    explicit Program(std::string start_label);

    /**
     * @brief Returns the block the cursor currently points at.
     * @return The current block.
     */
    BasicBlock& current() const { return *blocks_[current_]; }

    /**
     * @brief Looks up a block by its label, without moving the cursor.
     * @param label Label to look up.
     * @return The matching block. Behaviour is undefined if the label is not registered.
     */
    BasicBlock& get(const std::string& label);

    /**
     * @brief Inserts a new block relative to the cursor's current position, making it the new current block.
     * @param pos Position to insert at, relative to the current block.
     * @param block Block to insert.
     */
    void insert(Position pos, std::unique_ptr<BasicBlock> block);

    /**
     * @brief Moves the cursor to point at a specific block object, by identity.
     * @param block Block to select.
     * @return True if found and selected, false otherwise (leaving the cursor unchanged).
     */
    bool select(const BasicBlock& block);

    /**
     * @brief Moves the cursor to point at the block with a given label.
     * @param label Label to select.
     * @return True if found and selected, false otherwise (leaving the cursor unchanged).
     */
    bool select(const std::string& label);

    /**
     * @brief Moves the cursor by a relative navigation step.
     * @param pos Direction/position to move to, relative to the current block.
     */
    void select(Position pos);

    /**
     * @brief Pushes a new source-location context onto the location-tracking stack.
     * @param loc Location to push.
     */
    void add_location(Location loc);

    /**
     * @brief Returns the most recently pushed (innermost) source location.
     * @return The location, or empty if the stack is empty.
     */
    optional_ref<const Location> location() const;

    /**
     * @brief Pops the innermost source-location context, restoring the previous one.
     */
    void remove_location();

    /**
     * @brief Overwrites the current innermost location in place, or establishes one if the stack is empty.
     * @param loc Location to set.
     */
    void set_location(Location loc);

    /**
     * @brief Back-fills the origin of a range of lines in the current block.
     * @param origin Source location to attribute to the lines.
     * @param start Index of the first line to update; defaults to the most recently added line.
     * @param sudo If true, overwrites every line's origin unconditionally; otherwise only sets origins that are not already set.
     */
    void update_line_origins(const Location& origin, int start = -1, bool sudo = false) const;

    /**
     * @brief Back-fills line origins using the current top-of-stack location instead of a caller-supplied one.
     * @param start Index of the first line to update; defaults to the most recently added line.
     * @param sudo If true, overwrites every line's origin unconditionally; otherwise only sets origins that are not already set.
     */
    void update_line_origins(int start = -1, bool sudo = false) const;

    /**
     * @brief Renders the entire program: every block in order, separated by blank lines.
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    std::ostream& print(std::ostream& os) const;
  };
}
