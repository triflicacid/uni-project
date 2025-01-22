#pragma once

#include <set>
#include "lexer/lexer.hpp"
#include "messages/MessageWithSource.hpp"
#include "messages/list.hpp"
#include "ast/expr/literal.hpp"
#include "ast/symbol_declaration.hpp"
#include "ast/function.hpp"

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
    bool expect(lexer::TokenType type, unsigned int n = 0);

    // same as expect(types), but adds syntax error if failure
    bool expect_or_error(const std::set<lexer::TokenType>& types);
    bool expect_or_error(lexer::TokenType type);

    // consume the current token and return it
    lexer::Token consume();

    // generate a message of the given type about the current token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level);

    // generate_message() but at the given token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level, const lexer::Token& token);

    // generate an error message about a syntax error
    std::unique_ptr<message::MessageWithSource> generate_syntax_error(const std::set<lexer::TokenType>& expected_types);

    // parse a numeric literal
    std::unique_ptr<ast::expr::LiteralNode> parse_number();

    // parse a term - a number, symbol reference, bracketed expression etc.
    std::unique_ptr<ast::expr::Node> parse_term();

    // parse a type, return pointer to type, or nullptr if invalid
    const ast::type::Node* parse_type();

    // parse an expression
    std::unique_ptr<ast::expr::Node> parse_expression(int precedence = 0);

    // parse a `name: type` pair as a symbol declaration
    std::unique_ptr<ast::SymbolDeclarationNode> parse_name_type_pair();

    // parse a `let ...` statement
    std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> parse_let();

    // parse argument list: (arg ...)
    std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> parse_arg_list();

    // parse a function statement
    std::unique_ptr<ast::FunctionNode> parse_func();

    // parse a code block `{...}`
    std::unique_ptr<ast::BlockNode> parse_block();
  };

  // variables below contain the first sets for various structures
  namespace firstset {
    extern const lexer::TokenSet number;
    extern const lexer::TokenSet unary_operator;
    extern const lexer::TokenSet binary_operator;
    extern const lexer::TokenSet term;
    extern const lexer::TokenSet expression;
    extern const lexer::TokenSet type;
  }}
