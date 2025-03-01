#pragma once

#include <cstdint>
#include <string>

namespace lang::ast::type {
  class Node;
  class ArrayNode;
}

namespace lang::memory {
  // describe a literal - a word of memory + a type
  class Literal {
    const ast::type::Node& type_;
    uint64_t data_;

    Literal(const Literal&) = delete;

  protected:
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

    // return a Boolean literal true or false
    static const Literal& get_boolean(bool b);

    bool operator==(const Literal& other) const;
  };
}