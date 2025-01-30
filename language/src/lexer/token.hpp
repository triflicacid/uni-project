#pragma once

#include "location.hpp"
#include "token_type.hpp"
#include "messages/MessageWithSource.hpp"
#include <deque>

namespace lang::lexer {
  class Lexer;

  struct BasicToken {
    const TokenType type; // type of the token
    const std::string image; // the image which created us (source)

    explicit BasicToken(TokenType type) : type(type), image(token_type_to_string(type)) {}
    BasicToken(TokenType type, std::string image) : type(type), image(std::move(image)) {}

    size_t length() const { return image.size(); }
    bool is_eof() const;
    bool is_valid() const;

    std::string to_string() const;
  };

  struct Token : BasicToken {
    const std::string image_line; // file line which contains our image
    const Location source; // the location in which we were created
    uint64_t value = 0; // integer value associated with this token, used for literals

    Token(TokenType type, std::string image, std::string image_line, Location loc) : BasicToken(type, std::move(image)), image_line(std::move(image_line)), source(loc) {}

    // given numerical type TokenType (i.e., int8), parse image_ as a number of that type
    // return if parse was successful
    bool parse_numerical(TokenType type);

    // generate a message of the given type about this token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level) const;

    // generate an error message about a syntax error with this token
    std::unique_ptr<message::MessageWithSource> generate_syntax_error(const std::set<TokenType>& expected_types) const;

    // generate an error message about a syntax error with this token
    std::unique_ptr<message::MessageWithSource> generate_detailed_syntax_error(const std::deque<std::reference_wrapper<const BasicToken>>& expected_tokens) const;
  };
}
