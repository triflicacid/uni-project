#pragma once

#include "Type.hpp"
#include <iostream>

namespace language::types {
  class Byte : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 1; }

    [[nodiscard]] Category category() const override { return Category::Integer; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_BYTE]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<Byte />";
    }

    static const Byte instance;
  };

  class Int : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 4; }

    [[nodiscard]] Category category() const override { return Category::Integer; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_INT]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<Int />";
    }

    static const Int instance;
  };

  class UInt : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 4; }

    [[nodiscard]] Category category() const override { return Category::Integer; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_UINT]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<UInt />";
    }

    static const UInt instance;
  };

  class Word : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 8; }

    [[nodiscard]] Category category() const override { return Category::Integer; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_WORD]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<Word />";
    }

    static const Word instance;
  };

  class UWord : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 8; }

    [[nodiscard]] Category category() const override { return Category::Integer; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_UWORD]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<UWord />";
    }

    static const UWord instance;
  };

  class Float : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 4; }

    [[nodiscard]] Category category() const override { return Category::Float; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_FLOAT]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<Float />";
    }

    static const Float instance;
  };

  class Double : public Type {
    public:
    [[nodiscard]] size_t size() const override { return 8; }

    [[nodiscard]] Category category() const override { return Category::Float; }

    [[nodiscard]] std::string repr() const override { return lexer::tokens[lexer::Token::TYPE_DOUBLE]; }

    void debug_print(std::ostream& stream, const std::string& prefix) const override {
      stream << prefix << "<Double />";
    }

    static const Double instance;
  };

  /** return suitable type of integer (smallest possible). */
  const Type *get_suitable_int_type(uint64_t value);
}
