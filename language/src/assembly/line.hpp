#pragma once

#include <sstream>

namespace lang::assembly {
  class Line {
    std::stringstream comment_;

  protected:
    virtual std::ostream& _print(std::ostream& os) const = 0;

  public:
    virtual ~Line() = default;

    std::stringstream& comment() { return comment_; }

    std::ostream& print(std::ostream& os) const {
      _print(os);
      if (const std::string str = comment_.str(); !str.empty())
        os << "  ; " << str;
      return os;
    }
  };
}
