#pragma once

#include "message.hpp"

#include <string>

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
}
