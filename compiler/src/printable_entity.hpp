#pragma once

#include <string>

namespace lang {
  class PrintableEntity {
  public:
    virtual ~PrintableEntity() = default;

    virtual std::string node_name() const = 0;

    // print in code form
    virtual std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const = 0;
  };
}
