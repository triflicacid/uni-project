#pragma once

#include "location.hpp"
#include "token_type.hpp"
#include "messages/MessageWithSource.hpp"

namespace lang::lexer {
  class Lexer;

  struct Token {
    const TokenType type; // type of the token
    const std::string image; // the image which created us (source)
    const std::string image_line; // file line which contains our image
    const Location source; // the location in which we were created
    uint64_t value = 0; // integer value associated with this token, used for literals

    size_t length() const { return image.size(); }
    bool is_eof() const;
    bool is_valid() const;

    // id end of line (';' or '\n')
    bool is_eol() const;

    // generate a message of the given type about this token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level) const;

    // generate an error message about a syntax error with this token
    std::unique_ptr<message::MessageWithSource> generate_syntax_error(const TokenSet& expected_types) const;
  };
}
