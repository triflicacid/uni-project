#pragma once

#include <deque>
#include <string>
#include <memory>
#include <optional>
#include "line.hpp"

namespace lang::assembly {
  /**
   * @brief A single labelled sequence of lines (instructions/directives) that, by convention, may only end in a jump-type instruction.
   *
   * Private constructors, static-factory-only creation, and a deleted copy
   * constructor give every instance a single stable heap location, so it can be
   * safely referenced by address elsewhere (e.g. BlockReferenceArg, Program's
   * label map).
   */
  // a basic block represents a sequence of assembly instructions
  // it is labelled and can only contain jump instructions at the end
  class BasicBlock {
    std::string label_; ///< The block's label text.
    std::deque<std::unique_ptr<Line>> contents_; ///< Lines (instructions/directives) making up the block, in order.
    std::stringstream comment_; ///< Comment attached after the block's label, written to via @ref comment.
    std::optional<Location> origin_; ///< Source-code location this block originates from, if recorded via @ref origin.

    BasicBlock() {}
    explicit BasicBlock(std::string label) : label_(std::move(label)) {}

  public:
    BasicBlock(const BasicBlock&) = delete; // important as cannot copy unique_ptr

    /**
     * @brief Returns a mutable stream for attaching a comment to the block's label line.
     * @return The comment buffer stream.
     */
    std::stringstream& comment() { return comment_; }

    /**
     * @brief Returns this block's label text.
     * @return The label.
     */
    const std::string& label() const { return label_; }

    /**
     * @brief Appends one line (instruction or directive) to the end of the block.
     * @param i Line to append.
     * @return This block, for chaining.
     */
    BasicBlock& add(std::unique_ptr<Line> i);

    /**
     * @brief Returns how many lines the block currently contains.
     * @return The line count.
     */
    size_t size() const { return contents_.size(); }

    /**
     * @brief Returns the most recently added line.
     * @return The last line. Behaviour is undefined (asserted in debug builds) if the block is empty.
     */
    Line& back() const;

    /**
     * @brief Returns the first line in the block.
     * @return The first line. Behaviour is undefined (asserted in debug builds) if the block is empty.
     */
    Line& front() const;

    /**
     * @brief Returns the line at a given index.
     * @param i Index of the line.
     * @return The line at that index. Throws std::out_of_range if i is out of bounds.
     */
    Line& operator[](int i) { return *contents_.at(i); }

    /**
     * @brief Records the source-code location this whole block originates from.
     * @param loc Location to record.
     */
    void origin(Location loc) { origin_ = std::move(loc); }

    /**
     * @brief Renders the block: label header (with optional comment/origin decoration) followed by every contained line.
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    std::ostream& print(std::ostream& os) const;

    /**
     * @brief Creates a block with an automatically generated, guaranteed-unique label.
     * @return The newly created block.
     */
    // return a BasicBlock with a unique label
    static std::unique_ptr<BasicBlock> labelled();

    /**
     * @brief Creates a block with a caller-specified label.
     * @param label Label to assign; caller is responsible for avoiding collisions with existing labels.
     * @return The newly created block.
     */
    // create a BasicBlock with the given label
    static std::unique_ptr<BasicBlock> labelled(const std::string& label);

    /**
     * @brief Creates a block with no label at all.
     * @return The newly created block.
     */
    // create an unlabelled BasicBlock
    static std::unique_ptr<BasicBlock> unlabelled();
  };
}
