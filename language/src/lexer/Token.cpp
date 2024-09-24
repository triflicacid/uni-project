#include "Token.hpp"

namespace language::lexer {
  std::map<Token::Type, std::string> tokens = {
      {Token::KW_DECL,     "decl"},
      {Token::KW_DATA,     "data"},
      {Token::KW_ENTRY,    "entry"},
      {Token::KW_FUNC,     "func"},
      {Token::KW_RETURN,   "return"},

      {Token::TYPE_BYTE,   "byte"},
      {Token::TYPE_INT,    "int"},
      {Token::TYPE_UINT,   "uint"},
      {Token::TYPE_WORD,   "word"},
      {Token::TYPE_UWORD,  "uword"},
      {Token::TYPE_FLOAT,  "float"},
      {Token::TYPE_DOUBLE, "double"},

      {Token::UNIT,        "()"},

      {Token::LPARENS,     "("},
      {Token::RPARENS,     ")"},
      {Token::LBRACKET,    "["},
      {Token::RBRACKET,    "]"},
      {Token::LBRACE,      "{"},
      {Token::RBRACE,      "}"},

      {Token::COLON,       ":"},
      {Token::DOT,         "."},
      {Token::ARROW,       "->"},
      {Token::COMMA,       ","},
      {Token::EQUALS,      "="},
      {Token::PLUS,        "+"},
      {Token::DASH,        "-"},
      {Token::STAR,        "*"},
      {Token::SLASH,       "/"},
  };

  std::string Token::repr(Token::Type type) {
    switch (type) {
      case Token::DATA_IDENTIFIER:
        return "data identifier";
      case Token::IDENTIFIER:
        return "identifier";
      case Token::NUMBER:
        return "number";
      case Token::EOL:
        return "<EOL>";
      default:
        return "\"" + tokens.find(type)->second + "\"";
    }
  }
}
