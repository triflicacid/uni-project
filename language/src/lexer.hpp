#pragma once

#include <string>
#include "location.hpp"
#include "istream_wrapper.hpp"

namespace lang::lexer {
  enum class TokenType {
    ident = -1, // identifier name
    eof,

    lbrace, // {
    rbrace, // }
    lpar, // (
    rpar, // )
    sc, // ;
    comma, // ,
    dot, // .

    plus, // +
    minus, // -
    star, // *
    div, // /
    bang, // !
    assign, // =
    eq, // ==
    ne, // !=
    le, // <=
    lt, // <
    gt, // >
    ge, // >=
    land, // &&
    lor, // ||

    int_lit, // integer value, 'int'
    long_lit, // integer value, 'long'
    ulong_lit, // integer value, 'unsigned long'
    float_lit, // float value, 'float'
    double_lit, // float value, 'double'

    invalid
  };

  // given token type, return string representation
  std::string token_type_to_string(TokenType type, bool add_quotes = true);

  struct Token {
    TokenType type;
    std::string image;
    Location source;
    uint64_t value = 0;

    size_t length() const { return image.size(); }
    bool is_eof() const;
    bool is_valid() const;
  };

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
