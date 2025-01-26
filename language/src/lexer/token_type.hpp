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
    nl, // \n
    colon, // :
    comma, // ,
    dot, // .
    arrow, // ->

    // operators
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

    // values
    byte_lit, // integer value, 'unsigned char'
    int_lit, // integer value, 'int'
    long_lit, // integer value, 'long'
    ulong_lit, // integer value, 'unsigned long'
    float_lit, // float value, 'float'
    double_lit, // float value, 'double'

    // keywords
    byte_kw,
    double_kw,
    float_kw,
    func,
    int_kw,
    let,
    long_kw,
    namespace_kw,
    return_kw,
    ulong_kw,

    invalid
  };

  // given token type, return string representation
  std::string token_type_to_string(TokenType type, bool add_quotes = true);

  using TokenSet = std::set<TokenType>;

  TokenSet merge_sets(const std::vector<TokenSet>& sets);
}