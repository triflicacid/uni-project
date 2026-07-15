#pragma once

#include <string>
#include <set>
#include <vector>

namespace lang::lexer {
  /** @brief Kind of lexeme a token represents: punctuation, operator, built-in type name, literal, keyword, or the special `ident`/`eof`/`invalid` markers. */
  enum class TokenType {
    ident = -1, // identifier name
    eof,

    // punctuation
    lbrace, // {
    rbrace, // }
    lpar, // (
    rpar, // )
    lsquare, // [
    rsquare, // ]
    sc, // ;
    colon, // :
    comma, // ,

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
    break_kw,
    const_kw,
    continue_kw,
    else_kw,
    false_kw,
    func,
    if_kw,
    let,
    loop_kw,
    namespace_kw,
    null_kw,
    operator_kw,
    return_kw,
    sizeof_kw,
    struct_kw,
    true_kw,
    while_kw,

    invalid
  };

  /**
   * @brief Convert a token type to its human-readable string representation.
   * @param type Token type to describe.
   * @param add_quotes Whether to wrap the result in double quotes.
   * @return String naming the token type, or literal text for punctuation/keyword types.
   */
  std::string token_type_to_string(TokenType type, bool add_quotes = true);

  /** @brief Set of distinct token types, used to describe the set of tokens expected at a parse point. */
  using TokenTypeSet = std::set<TokenType>;

  /**
   * @brief Union a collection of token type sets into one set.
   * @param sets Sets to merge.
   * @return Union of all the given sets.
   */
  TokenTypeSet merge_sets(const std::vector<TokenTypeSet>& sets);
}