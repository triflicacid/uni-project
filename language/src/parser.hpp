#pragma once

#include <set>
#include "lexer.hpp"
#include "messages/MessageWithSource.hpp"
#include "messages/list.hpp"
#include "ast/expr/literal.hpp"

namespace lang::parser {
  class Parser {
    lexer::Lexer& lexer_;
    std::deque<lexer::Token> buffer_;
    message::List* messages_;

    // read at most n tokens from the lexer, stop on eof
    void read_tokens(unsigned int n);

    void add_message(std::unique_ptr<message::Message> m);

  public:
    explicit Parser(lexer::Lexer& lexer) : lexer_(lexer) {}

    const lexer::Lexer& lexer() const { return lexer_; }

    message::List* messages() { return messages_; }

    void messages(message::List* messages) { messages_ = messages; }

    // test if there was an error
    bool is_error() const;

    // peek n tokens into the future, default gets the next token
    const lexer::Token& peek(unsigned int n = 0);

    // same as peek(), but checks if the token type matches the given types
    bool expect(const std::set<lexer::TokenType>& types, unsigned int n = 0);

    // same as expect(types), but adds syntax error if failure
    bool expect_or_error(const std::set<lexer::TokenType>& types);

    // consume the current token and return it
    lexer::Token consume();

    // generate a message of the given type about the current token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level);

    // generate an error message about a syntax error
    std::unique_ptr<message::MessageWithSource> generate_syntax_error(const std::set<lexer::TokenType>& expected_types);

    // parse a numeric literal
    std::unique_ptr<ast::expr::LiteralNode> parse_number();

    // parse a term - a number, symbol reference, bracketed expression etc.
    std::unique_ptr<ast::expr::Node> parse_term();

    // parse an expression
    std::unique_ptr<ast::expr::Node> parse_expression(int precedence = 0);
  };

  // variables below contain the first sets for various structures
  namespace firstsets {
    extern const lexer::TokenSet number;
    extern const lexer::TokenSet unary_operator;
    extern const lexer::TokenSet binary_operator;
    extern const lexer::TokenSet term;
    extern const lexer::TokenSet expression;
  }}
