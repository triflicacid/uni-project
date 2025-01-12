#pragma once

#include <set>
#include "lexer.hpp"
#include "messages/MessageWithSource.hpp"
#include "messages/list.hpp"

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

    // consume the current token and return it
    lexer::Token consume();

    // consume the current token iff the token type matches, return if match
    bool consume(const std::set<lexer::TokenType>& types);

    // generate a message of the given type about the current token
    std::unique_ptr<message::MessageWithSource> generate_message(message::Level level);

    // generate an error message about a syntax error
    std::unique_ptr<message::MessageWithSource> generate_syntax_error(const std::set<lexer::TokenType>& expected_types);
  };
}
