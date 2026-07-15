#pragma once

#include "message.hpp"

#include <string>
#include <deque>

namespace message {
  /**
   * @brief A single-line diagnostic message that additionally underlines a span within its source line.
   *
   * Severity determines the underline/label color used when printing (see @ref print_notice,
   * @ref print_warning, @ref print_error).
   */
  class MessageWithSource : public Message {
    int len_; ///< Length of the underlined span.
    int idx_; ///< Index within `src_` the underlined span starts at.
    std::string src_; ///< Full text of the source line being underlined.

  public:
    /**
     * @brief Construct a message pointing at a span of a source line.
     * @param level Severity of the message.
     * @param loc Location of the start of the span.
     * @param idx Index within `src` that the span starts at.
     * @param len Length of the span to underline.
     * @param src Full text of the source line.
     */
    MessageWithSource(Level level, Location loc, int idx, int len, const std::string& src);

    /** @brief Print this message formatted as a note (using the note color scheme). */
    void print_notice() const;

    /** @brief Print this message formatted as a warning (using the warning color scheme). */
    void print_warning() const;

    /** @brief Print this message formatted as an error (using the error color scheme). */
    void print_error() const;

    /**
     * @brief Print the message, dispatching to @ref print_notice, @ref print_warning, or @ref print_error based on severity.
     * @param os Stream to print to.
     */
    void print(std::ostream& os) const override;
  };

  /** @brief A diagnostic message that spans and underlines a range of source covering multiple lines. */
  class MessageWithMultilineSource : public Message {
    Location end_; ///< End of the span; the span starts at `loc_` (inherited from @ref Message).
    std::deque<std::string> lines_; ///< Source text for every line from `loc_.line()` to `end_.line()`.

    /** @brief Pre-formatted text fragments (line-number gutter, primary underline, secondary underline) used when printing. */
    struct FormatInfo {
      std::string prefix; ///< Line-number gutter text.
      std::string primary; ///< Underline for the message's own span.
      std::string secondary; ///< Underline for surrounding context, if any.
    };

    /**
     * @brief Build the formatted fragments needed to render this message's underline.
     * @return The formatted fragments.
     */
    FormatInfo format() const;

  public:
    /**
     * @brief Construct a message spanning from `start` to `end`.
     * @param level Severity of the message.
     * @param start Location the span starts at.
     * @param end Location the span ends at.
     * @param lines Source text for every line from `start.line()` to `end.line()`.
     */
    MessageWithMultilineSource(Level level, Location start, Location end, std::deque<std::string> lines)
      : Message(level, std::move(start)), end_(std::move(end)), lines_(std::move(lines)) {}

    /**
     * @brief Print the message with its multi-line source span underlined.
     * @param os Stream to print to.
     */
    void print(std::ostream& os) const override;
  };
}
