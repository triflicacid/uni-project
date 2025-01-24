#pragma once

#include "token.hpp"
#include "istream_wrapper.hpp"
#include "messages/MessageWithSource.hpp"

namespace lang::lexer {
  class Lexer {
    IStreamWrapper& stream;

    // create a token with the given type and image
    Token token(const std::string& image, TokenType type) const;

  public:
    explicit Lexer(IStreamWrapper& stream) : stream(stream) {}

    std::string get_line(unsigned int line) const { return stream.get_line(line); }

    std::string get_source_name() const { return stream.get_name("<file>"); }

    bool is_eof() const { return stream.is_eof(); }

    // read & extract the next token from the input stream
    Token next();
  };
}
