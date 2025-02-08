#pragma once

#include <cstdint>
#include <string>

namespace lang::ast::type {
  class Node;
}

namespace lang::memory {
  class Literal {
    const ast::type::Node& type_;
    uint64_t data_;

    Literal(const ast::type::Node& type, uint64_t data) : type_(type), data_(data) {}

  public:
    const ast::type::Node& type() const { return type_; }

    uint64_t data() const { return data_; }

    // return string format of the numeric literal we represent
    std::string to_string() const;

    // get the following Literal object
    static const Literal& get(const ast::type::Node& type, uint64_t data);

    // get a zero constant of the given type
    static const Literal& zero(const ast::type::Node& type);
  };
}