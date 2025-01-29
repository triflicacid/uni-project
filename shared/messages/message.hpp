#pragma once

#include <string>
#include <utility>
#include <vector>
#include <optional>
#include "location.hpp"

namespace message {
  enum Level {
    Note,
    Warning,
    Error
  };

  class BasicMessage {
  protected:
    Level level_;
    std::stringstream msg_;

    /** Print varying type line e.g., 'ERROR!' */
    void print_type_suffix(std::ostream& os) const;

  public:
    explicit BasicMessage(Level level) : level_(level) {}

    std::stringstream& get() { return msg_; }

    /** Get message code, or -1. */
    virtual int get_code() const { return -1; }

    Level get_level() { return level_; }

    virtual void print(std::ostream& os) const;
  };

  class Message : public BasicMessage {
  protected:
    Location loc_;

  public:
    Message(Level level, Location loc) : BasicMessage(level), loc_(std::move(loc)) {}

    void print(std::ostream& os) const override;
  };

  /** Get level from int, where lowest is 0 */
  Level level_from_int(int level);
}
