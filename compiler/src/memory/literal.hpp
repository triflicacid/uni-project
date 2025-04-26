#pragma once

#include <cstdint>
#include <string>
#include "constants.hpp"

namespace lang::type {
  class Node;
  class ArrayNode;
}

namespace lang::memory {
  // describe a literal - a word of memory + a type
  class Literal {
    const type::Node& type_;
    uint64_t data_;

    Literal(const Literal&) = delete;

  protected:
    Literal(const type::Node& type, uint64_t data) : type_(type), data_(data) {}

  public:
    const type::Node& type() const { return type_; }

    uint64_t data() const { return data_; }

    // return string format of the numeric literal we represent
    std::string to_string() const;

    // change literal's type, returns copy (changes internal data to reflect datatype change)
    // works on basis of internal asm types
    const Literal& change_type(const type::Node& target) const;

    // get the following Literal object
    static const Literal& get(const type::Node& type, uint64_t data);

    // get a zero constant of the given type
    static const Literal& zero(const type::Node& type);

    // return a Boolean literal true or false
    static const Literal& get_boolean(bool b);

    bool operator==(const Literal& other) const;
  };
}