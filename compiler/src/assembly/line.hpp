#pragma once

#include <sstream>
#include <optional>
#include "location.hpp"
#include "config.hpp"

namespace lang::assembly {
  class Line {
    std::stringstream comment_;
    std::optional<Location> origin_;

  protected:
    virtual std::ostream& _print(std::ostream& os) const = 0;

  public:
    virtual ~Line() = default;

    std::stringstream& comment() { return comment_; }

    const std::optional<Location>& origin() const { return origin_; }

    void origin(Location loc) { origin_ = std::move(loc); }

    std::ostream& print(std::ostream& os) const {
      _print(os);
      if (const std::string str = comment_.str(); !str.empty())
        os << "  ; " << str;
      if (conf::debug && origin_) {
        os << "  ; ";
        origin_->print(os, true);
      }
      return os;
    }
  };
}
