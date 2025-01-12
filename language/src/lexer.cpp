#include <deque>
#include <unordered_map>
#include <iostream>
#include "lexer.hpp"

using namespace lang::lexer;

// map of token literals
static const std::deque<std::unordered_map<std::string, TokenType>>
  identifier_map,
  literal_map = {
    {
        {"==", TokenType::eq},
        {"!=", TokenType::ne},
        {"&&", TokenType::land},
        {"||", TokenType::lor},
        {"<=", TokenType::le},
        {">=", TokenType::ge}
    },
    {
        {"=", TokenType::assign},
        {"!", TokenType::bang},
        {"{", TokenType::lbrace},
        {"}", TokenType::rbrace},
        {")", TokenType::rpar},
        {"(", TokenType::lpar},
        {";", TokenType::sc},
        {",", TokenType::comma},
        {"+", TokenType::plus},
        {"-", TokenType::minus},
        {"*", TokenType::star},
        {"/", TokenType::div},
        {">", TokenType::gt},
        {"<", TokenType::lt},
        {".", TokenType::dot}
    }
  };

std::string lang::lexer::token_type_to_string(TokenType type, bool add_quotes) {
  // special case?
  switch (type) {
    case lang::lexer::TokenType::ident:
      return "ident";
    case lang::lexer::TokenType::int_lit:
      return "int_lit";
    case lang::lexer::TokenType::float_lit:
      return "float_lit";
    case lang::lexer::TokenType::eof:
      return "eof";
    case lang::lexer::TokenType::invalid:
      return "invalid";
    default: ;
  }

  // scan the identifier map
  for (auto& map : identifier_map)
    for (auto& [str, type2] : map)
      if (type2 == type)
        return add_quotes ? '"' + str + '"' : str;

  // scan the literal map
  for (auto& map : literal_map)
    for (auto& [str, type2] : map)
      if (type2 == type)
        return add_quotes ? '"' + str + '"' : str;

  return "unknown";
}

bool Token::is_eof() const {
  return type == TokenType::eof;
}

bool Token::is_valid() const {
  return type != TokenType::invalid;
}

Token Lexer::token(const std::string &image, TokenType type) const {
  const auto& position = stream.get_position();
  Token token{
    .type = type,
    .image = image,
    .source = Location(get_source_name(), position.line, position.col - image.length()),
  };

  // populate token's value if necessary
  switch (type) {
    case TokenType::int_lit: {
      int value = std::stoi(image);
      token.int_value = *(uint64_t*) &value;
      break;
    }
    case TokenType::long_lit: {
      int64_t value = std::stoll(image);
      token.int_value = *(uint64_t*) &value;
      break;
    }
    case TokenType::ulong_lit:
      token.int_value = std::stoull(image);
      break;
    case TokenType::float_lit:
      token.float_value = std::stof(image);
      break;
    case TokenType::double_lit:
      token.float_value = std::stod(image);
      break;
    default: ;
  }

  return token;
}

Token Lexer::next() {
  // eat leading whitespace, then check for eof
  stream.eat_whitespace();
  int ch = stream.peek_char();
  if (ch == EOF) return token("", TokenType::eof);

  // check for a comment
  if (ch == '/') {
    stream.save_position();
    stream.get_char();

    if (stream.peek_char() == ch) {
      // eat everything up to and including eol
      stream.eat_line();
      return next();
    } else {
      stream.restore_position();
    }
  }

  // scan for a number
  if (std::isdigit(ch) || ch == '.') {
    std::stringstream tmp;
    bool is_float = ch == '.';

    // eat initial digit sequence
    if (!is_float) {
      stream.eat_while(tmp, [](int ch) { return std::isdigit(ch); });
    }

    // have we a decimal point?
    if (is_float || stream.peek_char() == '.') {
      is_float = true;
      tmp << '.';
      stream.get_char();
      stream.eat_while(tmp, [](int ch) { return std::isdigit(ch); });
    }


    // only a single decimal point?
    std::string number = tmp.str();
    if (number == ".") {
      return token(".", TokenType::dot);
    }

    // create numeric token
    // TODO suppose long/ulong/double
    return token(number, is_float ? TokenType::float_lit : TokenType::int_lit);
  }

  // scan for an identifier
  if (std::isalpha(ch) || ch == '_') {
    // extract [0-9A-Za-z_]+
    std::stringstream tmp;
    stream.eat_while(tmp, [](int ch) { return std::isalnum(ch) || ch == '_'; });
    const std::string identifier = tmp.str();

    // check if this identifier is a keyword
    for (auto& map : identifier_map)
      for (auto& [target, type] : map)
        if (target == identifier)
          return token(target, type);

    // otherwise, return as generic identifier
    return token(identifier, TokenType::ident);
  }

  // search literal_map for a match
  for (auto& map : literal_map)
    for (auto& [literal, type] : map)
      if (stream.starts_with(literal))
        return token(literal, type);

  // invalid token, then
  return token(std::string(1, stream.get_char()), TokenType::invalid);
}
