#pragma once

#include <string>
#include <utility>
#include <vector>
#include <optional>
#include "location.hpp"

/** @brief Diagnostic messages (notes/warnings/errors) shared across the toolchain's components. */
namespace message {
  /** @brief Severity of a diagnostic message. */
  enum Level {
    Note, ///< Informational, non-erroneous message.
    Warning, ///< Potential issue that doesn't prevent compilation.
    Error ///< Issue that prevents compilation from succeeding.
  };

  /**
   * @brief A diagnostic message with a severity level and free-form text, without source location info.
   *
   * The message text is built up by writing to the stream returned by @ref get, then emitted via @ref print.
   */
  class BasicMessage {
  protected:
    Level level_; ///< Severity of the message.
    std::stringstream msg_; ///< Message text, written to via @ref get and read back by @ref print.

    /**
     * @brief Print the message's severity label (e.g. "error") and, if present, its code.
     * @param os Stream to print to.
     */
    void print_type_suffix(std::ostream& os) const;

  public:
    /** @brief Construct a message with a given severity. @param level Severity of the message. */
    explicit BasicMessage(Level level) : level_(level) {}

    /** @brief Get the stream to write this message's text into. @return Reference to the message's text buffer. */
    std::stringstream& get() { return msg_; }

    /** @brief Get this message's diagnostic code, if it has one. @return The code, or -1 if none. */
    virtual int get_code() const { return -1; }

    /** @brief Get the message's severity. @return The severity level. */
    Level get_level() { return level_; }

    /**
     * @brief Print the full message (severity label, code, and text) to a stream.
     * @param os Stream to print to.
     */
    virtual void print(std::ostream& os) const;
  };

  /** @brief A diagnostic message attributed to a specific source location. */
  class Message : public BasicMessage {
  protected:
    Location loc_; ///< Source location this message is attributed to.

  public:
    /**
     * @brief Construct a message at a given location.
     * @param level Severity of the message.
     * @param loc Source location the message refers to.
     */
    Message(Level level, Location loc) : BasicMessage(level), loc_(std::move(loc)) {}

    /**
     * @brief Print the location followed by the message (severity label, code, and text) to a stream.
     * @param os Stream to print to.
     */
    void print(std::ostream& os) const override;
  };

  /**
   * @brief Map an integer to a severity level, clamping to the valid range.
   * @param level Integer level, where the lowest value (below 1) maps to @ref message::Note.
   * @return The corresponding severity level.
   */
  Level level_from_int(int level);

  /** @brief Interface for types that can produce a diagnostic @ref Message for themselves, at a given severity. */
  struct MessageGenerator {
    /**
     * @brief Build a message describing this object.
     * @param lvl Severity to report the message at.
     * @return The generated message.
     */
    virtual std::unique_ptr<message::Message> generate_message(Level lvl) const = 0;
  };
}
