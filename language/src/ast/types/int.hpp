#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::ast::type {
  class IntNode : public Node {
    bool signed_;
    uint8_t width_; // width of integer in bytes

  public:
    IntNode(uint8_t width, bool is_signed) : signed_(is_signed), width_(width) {}

    const IntNode* get_int() const override { return this; }

    std::string name() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return width_; }

    std::strong_ordering operator<=>(const IntNode& other) const {
      if (width_ == other.width_) {
        return !signed_ <=> other.signed_;
      }
      return width_ <=> other.width_;
    }

    uint8_t width() const { return width_; }

    bool is_signed() const { return signed_; }

    constants::inst::datatype::dt get_asm_datatype() const;
  };

  extern IntNode uint8, int8;
  extern IntNode uint16, int16;
  extern IntNode uint32, int32;
  extern IntNode uint64, int64;
}
