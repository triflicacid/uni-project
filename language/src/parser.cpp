#include <iostream>
#include "parser.hpp"

void lang::parser::Parser::read_tokens(unsigned int n) {
  for (unsigned int i = 0; i < n; i++) {
    buffer_.emplace_back(lexer_.next());
    if (buffer_.back().is_eof()) break;
  }
}

void lang::parser::Parser::add_message(std::unique_ptr<message::Message> m) {
  if (m && messages_) messages_->add(std::move(m));
}

bool lang::parser::Parser::is_error() const {
  return messages_ && messages_->has_message_of(message::Error);
}

const lang::lexer::Token &lang::parser::Parser::peek(unsigned int n) {
  // ensure buffer contains sufficient number of tokens
  if (size_t size = buffer_.size(); size <= n) {
    read_tokens(n - size + 1);
  }

  // if we are still empty, return the last token (which will be eof)
  if (buffer_.size() < n) {
    return buffer_.back();
  }

  return buffer_[n];
}

bool lang::parser::Parser::expect(const std::set<lexer::TokenType> &types, unsigned int n) {
  return types.find(peek(n).type) != types.end();
}

lang::lexer::Token lang::parser::Parser::consume() {
  if (buffer_.empty()) read_tokens(1);
  lexer::Token& last = buffer_.front();
  buffer_.pop_front();
  return last;
}

bool lang::parser::Parser::consume(const std::set<lexer::TokenType> &types) {
  if (expect(types)) {
    consume();
    return true;
  }
  return false;
}

std::unique_ptr<message::MessageWithSource>
lang::parser::Parser::generate_message(message::Level level) {
  const lexer::Token& token = peek();
  int end_column = token.source.column() + token.length();

  return std::make_unique<message::MessageWithSource>(
      level,
      token.source.copy().column(end_column),
      token.source.column() - 1,
      end_column - token.source.column(),
      lexer_.get_line(token.source.line())
    );
}

std::unique_ptr<message::MessageWithSource>
lang::parser::Parser::generate_syntax_error(const std::set<lexer::TokenType> &expected_types) {
  auto error = generate_message(message::Error);

  // message: "syntax error: encountered <type>, expected [one of] <type1>[, <type2>[, ...]]"
  std::stringstream& stream = error->get();
  stream << "syntax error: encountered " << lexer::token_type_to_string(peek().type);
  if (!expected_types.empty()) {
    stream << ", expected ";
    if (expected_types.size() > 1) stream << " one of ";
    int i = 0;
    for (const lexer::TokenType& type : expected_types) {
      stream << lexer::token_type_to_string(type);
      if (++i < expected_types.size()) stream << ", ";
    }
  }
  stream << '.';

  return error;
}
