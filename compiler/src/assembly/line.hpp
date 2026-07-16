#pragma once

#include <sstream>
#include <optional>
#include "location.hpp"
#include "config.hpp"

namespace lang::assembly {
  /**
   * @brief Abstract base for anything that can live inside a BasicBlock (an instruction or a directive).
   *
   * Owns the shared trailing-comment and source-origin bookkeeping, and implements
   * the single non-virtual print() entry point; subclasses only need to implement
   * the content-rendering hook _print().
   */
  class Line {
    std::stringstream comment_; ///< Trailing human-readable comment, written to via @ref comment.
    std::optional<Location> origin_; ///< Source-code location this line was generated from, if recorded via @ref origin.

  protected:
    /**
     * @brief Renders this line's own content (mnemonic/operands or directive name/data), with no comment or origin suffix.
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    virtual std::ostream& _print(std::ostream& os) const = 0;

  public:
    virtual ~Line() = default;

    /**
     * @brief Returns a mutable stream for attaching a trailing human-readable comment to this line.
     * @return The comment buffer stream.
     */
    std::stringstream& comment() { return comment_; }

    /**
     * @brief Returns the source-code location this line was generated from, if tracked.
     * @return The origin, or empty if none is recorded.
     */
    const std::optional<Location>& origin() const { return origin_; }

    /**
     * @brief Records (or overwrites) the source-code location this line originated from.
     * @param loc Location to record.
     */
    void origin(Location loc) { origin_ = std::move(loc); }

    /**
     * @brief Renders the line's content, followed by its trailing comment and (in debug mode) its source origin.
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    std::ostream& print(std::ostream& os) const {
      _print(os);
      if (const std::string str = comment_.str(); !str.empty())
        os << "  ; " << str;
      if (conf::debug && origin_) {
        os << "  ;@";
        origin_->print(os, true);
      }
      return os;
    }
  };
}
