#include <deque>
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include "uint64.hpp"
#include "lexer.hpp"
#include "token.hpp"

using namespace lang::lexer;

// map of token literals
static const std::deque<std::unordered_map<std::string, TokenType>>
  identifier_map = {{
      {"u8", TokenType::uint8},
      {"byte", TokenType::uint8},
      {"i8", TokenType::int8},
      {"u16", TokenType::uint16},
      {"i16", TokenType::int16},
      {"u32", TokenType::uint32},
      {"i32", TokenType::int32},
      {"int", TokenType::int32},
      {"u64", TokenType::uint64},
      {"i64", TokenType::int64},
      {"long", TokenType::int64},
      {"f32", TokenType::float32},
      {"float", TokenType::float32},
      {"f64", TokenType::float64},
      {"double", TokenType::float64},
    },
    {
      {"func", TokenType::func},
      {"let", TokenType::let},
      {"namespace", TokenType::namespace_kw},
      {"return", TokenType::return_kw},
    }
  },
  literal_map = {
    {
        {"->", TokenType::arrow},
    },
    {
        {"{", TokenType::lbrace},
        {"}", TokenType::rbrace},
        {")", TokenType::rpar},
        {"(", TokenType::lpar},
        {";", TokenType::sc},
        {":", TokenType::colon},
        {",", TokenType::comma},
    }
  };

std::string lang::lexer::token_type_to_string(TokenType type, bool add_quotes) {
  // special case?
  switch (type) {
    case TokenType::ident:
      return "ident";
    case TokenType::eof:
      return "eof";
    case TokenType::op:
      return "operator";
    case TokenType::int_lit:
      return "int";
    case TokenType::float_lit:
      return "float";
    case TokenType::nl:
      return "newline";
    case TokenType::invalid:
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

TokenSet lang::lexer::merge_sets(const std::vector<TokenSet> &sets) {
  TokenSet result;
  for (const auto &set : sets) {
    result.insert(set.begin(), set.end());
  }

  return result;
}

bool Token::is_eof() const {
  return type == TokenType::eof;
}

bool Token::is_valid() const {
  return type != TokenType::invalid;
}

std::unique_ptr<message::MessageWithSource> Token::generate_message(message::Level level) const {
  int end_column = source.column() + length();
  return std::make_unique<message::MessageWithSource>(
      level,
      source.copy().column(end_column),
      source.column() - 1,
      end_column - source.column(),
      image_line
  );
}

std::unique_ptr<message::MessageWithSource> Token::generate_syntax_error(const TokenSet& expected_types) const {
  auto error = generate_message(message::Error);

  // message: "syntax error: encountered <type>, expected [one of] <type1>[, <type2>[, ...]]"
  std::stringstream& stream = error->get();
  stream << "syntax error: encountered " << lexer::token_type_to_string(type);
  if (!expected_types.empty()) {
    stream << ", expected ";
    if (expected_types.size() > 1) stream << "one of ";
    int i = 0;
    for (const lexer::TokenType& expected : expected_types) {
      stream << lexer::token_type_to_string(expected);
      if (++i < expected_types.size()) stream << ", ";
    }
  }
  stream << '.';

  return error;
}

bool Token::parse_numerical(TokenType num_type) {
  switch (num_type) {
    case TokenType::uint8:
      value = uint64::from(static_cast<uint8_t>(std::stoul(image)));
      break;
    case TokenType::int8:
      value = uint64::from(static_cast<int8_t>(std::stoi(image)));
      break;
    case TokenType::uint16:
      value = uint64::from(static_cast<uint16_t>(std::stoul(image)));
      break;
    case TokenType::int16:
      value = uint64::from(static_cast<int16_t>(std::stoi(image)));
      break;
    case TokenType::uint32:
      value = uint64::from(static_cast<uint32_t>(std::stoul(image)));
      break;
    case TokenType::int32:
      value = uint64::from(static_cast<int32_t>(std::stoi(image)));
      break;
    case TokenType::uint64:
      value = uint64::from(static_cast<uint64_t>(std::stoull(image)));
      break;
    case TokenType::int64:
      value = uint64::from(static_cast<int64_t>(std::stoll(image)));
      break;
    case TokenType::float32:
      value = uint64::from(std::stof(image));
      break;
    case TokenType::float64:
      value = uint64::from(std::stod(image));
      break;
    default:
      return false;
  }
  return true;
}

Token Lexer::token(const std::string &image, TokenType type) const {
  const auto& position = stream.get_position();
  return Token{
    .type = type,
    .image = image,
    .image_line = get_line(position.line),
    .source = Location(get_source_name(), position.line, position.col - image.length()),
  };
}

// characters which may be used for an operator
// inspired by the Haskell report (https://www.haskell.org/onlinereport/lexemes.html)
static std::unordered_set<char> operator_chars = {'!', '#', '$', '%', '&', '*', '+', '.', '/', '<', '=', '>', '?', '@', '\\', '^', '|', '-', '~'};

// used to avoid translating `\r\n` into two newlines
static bool seen_CR = false;

Token Lexer::next() {
  // eat leading whitespace, then check for eof
  stream.eat_whitespace();
  int ch = stream.peek_char();
  if (ch == EOF) return token("", TokenType::eof);
//  if (ch == '\n' || ch == '\r') {
//    stream.get_char(); // skip past newline
//    if (seen_CR && ch == '\n') { // if we've already found '\r', ignore '\n' and continue to next token
//      seen_CR = false;
//      return next();
//    }
//    seen_CR = ch == '\r';
//    return token("", TokenType::nl);
//  }

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
  if (std::isdigit(ch)) {
    std::stringstream tmp;
    bool is_float = false;

    // eat initial digit sequence
    stream.eat_while(tmp, [](int ch) { return std::isdigit(ch); });

    // have we a decimal point?
    if (stream.peek_char() == '.') {
      is_float = true;
      tmp << '.';
      stream.get_char();
      stream.eat_while(tmp, [](int ch) { return std::isdigit(ch); });
    }

    // create numeric token
    return token(tmp.str(), is_float ? TokenType::float_lit : TokenType::int_lit);
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

  // scan for an operator
  // note, this is after to allow some operators to be parsed as other structures
  if (operator_chars.contains(ch)) {
    std::stringstream tmp;
    stream.eat_while(tmp, [&](int ch) { return operator_chars.contains(ch); });
    return token(tmp.str(), TokenType::op);
  }

  // invalid token, then
  return token(std::string(1, stream.get_char()), TokenType::invalid);
}
