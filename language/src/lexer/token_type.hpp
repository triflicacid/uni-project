#pragma once

#include <string>
#include <set>
#include <vector>

namespace lang::lexer {
  enum class TokenType {
    ident = -1, // identifier name
    eof,

    // punctuation
    lbrace, // {
    rbrace, // }
    lpar, // (
    rpar, // )
    sc, // ;
    colon, // :
    comma, // ,
    arrow, // ->

    // operators
    op, // generic operator

    // types
    boolean,
    uint8,
    int8,
    uint16,
    int16,
    uint32,
    int32,
    uint64,
    int64,
    float32,
    float64,

    // literals
    int_lit, // any integer literal
    float_lit, // any floating literal

    // keywords
    as_kw,
    const_kw,
    false_kw,
    func,
    let,
    namespace_kw,
    operator_kw,
    registerof_kw,
    return_kw,
    true_kw,

    invalid
  };

  // given token type, return string representation
  std::string token_type_to_string(TokenType type, bool add_quotes = true);

  using TokenTypeSet = std::set<TokenType>;

  TokenTypeSet merge_sets(const std::vector<TokenTypeSet>& sets);
}