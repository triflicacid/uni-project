#include <iostream>
#include <map>
#include <ranges>
#include "parser.hpp"
#include "ast/types/float.hpp"
#include "ast/types/int.hpp"
#include "ast/expr/operator.hpp"
#include "ast/expr/symbol_reference.hpp"
#include "util.hpp"

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

bool lang::parser::Parser::expect(lang::lexer::TokenType type, unsigned int n) {
  return peek(n).type == type;
}

bool lang::parser::Parser::expect_or_error(const std::set<lexer::TokenType> &types) {
  if (expect(types)) {
    return true;
  }

  add_message(std::move(generate_syntax_error(types)));
  return false;
}

bool lang::parser::Parser::expect_or_error(lang::lexer::TokenType type) {
  if (expect(type)) {
    return true;
  }

  add_message(std::move(generate_syntax_error({type})));
  return false;
}

lang::lexer::Token lang::parser::Parser::consume() {
  if (buffer_.empty()) read_tokens(1);
  lexer::Token& last = buffer_.front();
  buffer_.pop_front();
  return last;
}

std::unique_ptr<message::MessageWithSource>
lang::parser::Parser::generate_message(message::Level level, const lang::lexer::Token& token) {
  int end_column = token.source.column() + token.length();

  return std::make_unique<message::MessageWithSource>(
      level,
      token.source.copy().column(end_column),
      token.source.column() - 1,
      end_column - token.source.column(),
      lexer_.get_line(token.source.line())
  );
}

std::unique_ptr<message::MessageWithSource> lang::parser::Parser::generate_message(message::Level level) {
  return generate_message(level, peek());
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

const lang::lexer::TokenSet lang::parser::firstset::number{lexer::TokenType::int_lit, lexer::TokenType::float_lit};

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

const lang::lexer::TokenSet lang::parser::firstset::unary_operator(unary_operators_view.begin(), unary_operators_view.end());
const lang::lexer::TokenSet lang::parser::firstset::binary_operator(binary_operators_view.begin(), binary_operators_view.end());
const lang::lexer::TokenSet lang::parser::firstset::term = lexer::merge_sets({
                                                                                 firstset::number,
                                                                                 {lexer::TokenType::ident, lexer::TokenType::lpar}
});

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::parse_term() {
  if (expect(firstset::number)) { // numeric literal
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

const lang::lexer::TokenSet lang::parser::firstset::expression = lexer::merge_sets({
                                                                                       firstset::term,
                                                                                       firstset::unary_operator
});

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::parse_expression(int precedence) {
  std::unique_ptr<ast::expr::Node> expr;

  // check if we have a unary operator, or just a term
  if (expect(firstset::unary_operator)) {
    lexer::Token op_token = consume();
    // check if there is a term following this
    if (!expect(firstset::term)) {
      add_message(std::move(generate_syntax_error(firstset::term)));
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

  while (expect(firstset::binary_operator)) {
    lexer::Token op_token = peek();
    auto& op_info = token_to_binary_operator_map[op_token.type];

    // exit if lower precedence
    if (precedence >= op_info.precedence) {
      break;
    }

    // parse RHS of expression, supplying the operator's precedence as a new baseline
    consume();
    if (!expect_or_error(firstset::expression)) return nullptr;
    int new_precedence = op_info.precedence;
    if (op_info.right_associative) new_precedence--;
    auto rest = parse_expression(new_precedence);
    if (is_error()) return nullptr;

    // wrap both sides in a binary operator
    expr = std::make_unique<ast::expr::BinaryOperatorNode>(op_token, op_info.type, std::move(expr), std::move(rest));
  }

  return expr;
}

const lang::lexer::TokenSet lang::parser::firstset::type = {
    lexer::TokenType::byte_kw,
    lexer::TokenType::int_kw,
    lexer::TokenType::long_kw,
};

const lang::ast::type::Node* lang::parser::Parser::parse_type() {
  if (expect(lexer::TokenType::byte_kw)) {
    consume();
    return &ast::type::uint8;
  }

  if (expect(lexer::TokenType::int_kw)) {
    consume();
    return &ast::type::int32;
  }

  if (expect(lexer::TokenType::long_kw)) {
    consume();
    return &ast::type::int64;
  }

  return nullptr;
}

std::unique_ptr<lang::ast::SymbolDeclarationNode> lang::parser::Parser::parse_name_type_pair() {
  // name
  lexer::Token name = consume();

  // ':'
  if (!expect_or_error(lexer::TokenType::colon)) return nullptr;
  consume();

  // type
  if (!expect_or_error(firstset::type)) return nullptr;
  const ast::type::Node* type = parse_type();
  if (is_error() || !type) return nullptr;

  return std::make_unique<ast::SymbolDeclarationNode>(std::move(name), *type);
}

std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> lang::parser::Parser::parse_let() {
  // consume 'let'
  consume();

  std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> declarations;
  while (true) {
    // parse `name: type` pair
    if (!expect_or_error(lexer::TokenType::ident)) return {};
    declarations.push_back(parse_name_type_pair());
    if (is_error()) return {};

    // if comma, continue
    if (!expect(lexer::TokenType::comma)) break;
    consume();
  }

  return declarations;
}

std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> lang::parser::Parser::parse_arg_list() {
  // '('
  const lexer::Token lpar = consume();

  std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> args;
  while (true) {
    // parse `name: type` pair
    if (!expect_or_error(lexer::TokenType::ident)) return {};
    args.push_back(parse_name_type_pair());
    if (is_error()) return {};

    // if comma, continue
    if (!expect(lexer::TokenType::comma)) break;
    consume();
  }

  // ')'
  if (!expect_or_error(lexer::TokenType::rpar)) {
    auto msg = generate_message(message::Note, lpar);
    msg->get() << "parenthesis opened here";
    add_message(std::move(msg));
    return {};
  }

  consume();
  return args;
}

std::unique_ptr<lang::ast::FunctionNode> lang::parser::Parser::parse_func() {
  // 'func'
  consume();

  // name
  if (!expect_or_error(lexer::TokenType::ident)) return nullptr;
  const lexer::Token name = consume();

  // parse any supplied arguments
  std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> params;
  if (expect(lexer::TokenType::lpar)) {
    params = parse_arg_list();
    if (is_error()) return nullptr;
  }

  // parse an optional return type
  std::optional<std::reference_wrapper<const ast::type::Node>> returns;
  if (expect(lexer::TokenType::arrow)) {
    consume();
    if (!expect_or_error(firstset::type)) return nullptr;
    auto type = parse_type();
    if (is_error() || !type) return nullptr;
    returns = *type;
  }

  // parse function body (optional)
  if (!expect_or_error({lexer::TokenType::sc, lexer::TokenType::lbrace})) return nullptr;
  std::optional<std::unique_ptr<ast::BlockNode>> body;
  if (expect(lexer::TokenType::lbrace)) {
    body = parse_block();
    if (is_error()) return nullptr;
  }

  // construct function type and node
  std::deque<std::reference_wrapper<const ast::type::Node>> param_types;
  for (auto& param : params) param_types.push_back(param->type());
  auto type = std::make_unique<ast::type::FunctionNode>(param_types, returns);
  return std::make_unique<ast::FunctionNode>(name, std::move(type), std::move(params), std::move(body));
}

std::unique_ptr<lang::ast::BlockNode> lang::parser::Parser::parse_block() {
  // '{'
  const lexer::Token lbrace = consume();

  auto block = std::make_unique<ast::BlockNode>(lbrace);
  // TODO ...

  // '}'
  if (!expect_or_error(lexer::TokenType::rbrace)) {
    auto msg = generate_message(message::Note, lbrace);
    msg->get() << "block started here";
    add_message(std::move(msg));
    return nullptr;
  }

  consume();
  return block;
}
