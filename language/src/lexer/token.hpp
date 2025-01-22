#pragma once

#include "location.hpp"
#include "token_type.hpp"

namespace lang::lexer {
  struct Token {
    TokenType type;
    std::string image;
    Location source;
    uint64_t value = 0;

    size_t length() const { return image.size(); }
    bool is_eof() const;
    bool is_valid() const;

    // id end of line (';' or '\n')
    bool is_eol() const;
  };
}
