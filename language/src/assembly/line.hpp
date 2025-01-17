#pragma once

namespace lang::assembly {
  struct Line {
    virtual std::ostream& print(std::ostream& os) const = 0;
  };
}
