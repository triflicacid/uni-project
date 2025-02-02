#pragma once

#include "message.hpp"

#include <string>
#include <deque>

/** A more in-depth message class able to handle source information. No separate error class. */
namespace message {
  class MessageWithSource : public Message {
    int len_; // Length of the error
    int idx_; // Index to point to
    std::string src_;

  public:
    MessageWithSource(Level level, Location loc, int idx, int len, const std::string& src);

    void print_notice() const;

    void print_warning() const;

    void print_error() const;

    void print(std::ostream& os) const override;
  };

  class MessageWithMultilineSource : public Message {
    Location end_; // starts at loc_, ends here
    std::deque<std::string> lines_;

    struct FormatInfo {
      std::string prefix;
      std::string primary;
      std::string secondary;
    };

    FormatInfo format() const;

  public:
    // lines start.line() to end.line()
    MessageWithMultilineSource(Level level, Location start, Location end, std::deque<std::string> lines)
      : Message(level, std::move(start)), end_(std::move(end)), lines_(std::move(lines)) {}

    void print(std::ostream& os) const override;
  };
}
