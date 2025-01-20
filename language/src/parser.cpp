#include <iostream>
#include <map>
#include <ranges>
#include "parser.hpp"
#include "ast/types/float.hpp"
#include "ast/types/int.hpp"
#include "ast/expr/operator.hpp"
#include "ast/expr/symbol_reference.hpp"

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

bool lang::parser::Parser::expect_or_error(const std::set<lexer::TokenType> &types) {
  if (expect(types)) {
    return true;
  }

  add_message(std::move(generate_syntax_error(types)));
  return false;
}

lang::lexer::Token lang::parser::Parser::consume() {
  if (buffer_.empty()) read_tokens(1);
  lexer::Token& last = buffer_.front();
  buffer_.pop_front();
  return last;
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
    if (expected_types.size() > 1) stream << "one of ";
    int i = 0;
    for (const lexer::TokenType& type : expected_types) {
      stream << lexer::token_type_to_string(type);
      if (++i < expected_types.size()) stream << ", ";
    }
  }
  stream << '.';

  return error;
}

const lang::lexer::TokenSet lang::parser::firstsets::number{lexer::TokenType::int_lit, lexer::TokenType::float_lit};

std::unique_ptr<lang::ast::expr::LiteralNode> lang::parser::Parser::parse_number() {
  // TODO determine types properly
  // assume token is float_lit or int_lit
  const lexer::Token& token = consume();
  ast::type::Node* type_node;

  if (token.type == lexer::TokenType::float_lit) {
    type_node = &ast::type::float32;
  } else {
    type_node = &ast::type::int32;
  }

  return std::make_unique<ast::expr::LiteralNode>(token, *type_node);
}

// map of token types to operators
static std::map<lang::lexer::TokenType, lang::ast::expr::OperatorInfo> token_to_binary_operator_map = {
    {lang::lexer::TokenType::assign, {4, true, lang::ast::expr::OperatorType::ASSIGNMENT}},
    {lang::lexer::TokenType::plus, {14, false, lang::ast::expr::OperatorType::ADDITION}},
    {lang::lexer::TokenType::minus, {14, false, lang::ast::expr::OperatorType::SUBTRACTION}},
    {lang::lexer::TokenType::star, {15, false, lang::ast::expr::OperatorType::MULTIPLICATION}},
    {lang::lexer::TokenType::div, {15, false, lang::ast::expr::OperatorType::DIVISION}},
    {lang::lexer::TokenType::dot, {15, false, lang::ast::expr::OperatorType::MEMBER_ACCESS}},
}, token_to_unary_operator_map = {
    {lang::lexer::TokenType::minus, {17, false, lang::ast::expr::OperatorType::MINUS}}
};

static const auto binary_operators_view = std::views::keys(token_to_binary_operator_map);
static const auto unary_operators_view = std::views::keys(token_to_unary_operator_map);

const lang::lexer::TokenSet lang::parser::firstsets::unary_operator(unary_operators_view.begin(), unary_operators_view.end());
const lang::lexer::TokenSet lang::parser::firstsets::binary_operator(binary_operators_view.begin(), binary_operators_view.end());
const lang::lexer::TokenSet lang::parser::firstsets::term = lexer::merge_sets({
    firstsets::number,
    {lexer::TokenType::ident, lexer::TokenType::lpar}
});

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::parse_term() {
  if (expect(firstsets::number)) { // numeric literal
    return parse_number();
  } else if (expect({lexer::TokenType::ident})) { // identifier
    return std::make_unique<ast::expr::SymbolReferenceNode>(consume());
  } else if (expect({lexer::TokenType::lpar})) { // bracketed expression
    consume();
    auto expr = parse_expression();
    // ensure we have a closing bracket, or propagate error
    if (is_error() || !expect_or_error({lexer::TokenType::rpar})) return nullptr;
    consume(); // ')'
    return expr;
  } else { // unknown, but should've been caught already
    return nullptr;
  }
}

const lang::lexer::TokenSet lang::parser::firstsets::expression = lexer::merge_sets({
    firstsets::term,
    firstsets::unary_operator
});

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::parse_expression(int precedence) {
  std::unique_ptr<ast::expr::Node> expr;

  // check if we have a unary operator, or just a term
  if (expect(firstsets::unary_operator)) {
    lexer::Token op_token = consume();
    // check if there is a term following this
    if (!expect(firstsets::term)) {
      add_message(std::move(generate_syntax_error(firstsets::term)));
      return nullptr;
    }

    std::unique_ptr<ast::expr::Node> term = parse_term();
    // propagate error if necessary
    if (is_error()) return nullptr;
    // wrap term in the unary operator
    return std::make_unique<ast::expr::UnaryOperatorNode>(op_token, token_to_unary_operator_map[op_token.type].type,
                                                          std::move(term));
  } else {
    expr = parse_term();
    if (is_error()) return nullptr;
  }

  while (expect(firstsets::binary_operator)) {
    lexer::Token op_token = peek();
    auto& op_info = token_to_binary_operator_map[op_token.type];

    // exit if lower precedence
    if (precedence >= op_info.precedence) {
      break;
    }

    // parse RHS of expression, supplying the operator's precedence as a new baseline
    consume();
    if (!expect_or_error(firstsets::expression)) return nullptr;
    int new_precedence = op_info.precedence;
    if (op_info.right_associative) new_precedence--;
    auto rest = parse_expression(new_precedence);
    if (is_error()) return nullptr;

    // wrap both sides in a binary operator
    expr = std::make_unique<ast::expr::BinaryOperatorNode>(op_token, op_info.type, std::move(expr), std::move(rest));
  }

  return expr;
}
