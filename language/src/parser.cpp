#include <iostream>
#include <map>
#include <ranges>
#include <cassert>
#include "parser.hpp"
#include "ast/block.hpp"
#include "ast/types/float.hpp"
#include "ast/types/int.hpp"
#include "ast/expr/operator.hpp"
#include "ast/expr/symbol_reference.hpp"
#include "util.hpp"
#include "ast/program.hpp"
#include "operators/info.hpp"
#include "ast/expr.hpp"
#include "ast/types/bool.hpp"
#include "config.hpp"

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

bool lang::parser::Parser::expect(const std::deque<std::reference_wrapper<const lexer::BasicToken>>& tokens, unsigned int n) {
  auto& next = peek(n);
  return std::any_of(tokens.begin(), tokens.end(), [&next](auto& token) {
    return next.type == token.get().type && next.image == token.get().image;
  });
}

bool lang::parser::Parser::expect(const lang::lexer::BasicToken& token, unsigned int n) {
  auto& next = peek(n);
  return next.type == token.type && next.image == token.image;
}

bool lang::parser::Parser::expect_or_error(const std::set<lexer::TokenType> &types) {
  if (expect(types)) {
    return true;
  }

  add_message(std::move(peek().generate_syntax_error(types)));
  return false;
}

bool lang::parser::Parser::expect_or_error(lang::lexer::TokenType type) {
  if (expect(type)) {
    return true;
  }

  add_message(std::move(peek().generate_syntax_error({type})));
  return false;
}

bool lang::parser::Parser::expect_or_error(const std::deque<std::reference_wrapper<const lexer::BasicToken>>& tokens) {
  if (expect(tokens)) {
    return true;
  }

  add_message(std::move(peek().generate_detailed_syntax_error(tokens)));
  return false;
}

bool lang::parser::Parser::expect_or_error(const lang::lexer::BasicToken& token) {
  if (expect(token)) {
    return true;
  }

  add_message(std::move(peek().generate_detailed_syntax_error({token})));
  return false;
}

lang::lexer::Token lang::parser::Parser::consume() {
  if (buffer_.empty()) read_tokens(1);
  const lexer::Token last = buffer_.front();
  buffer_.pop_front();
  return last;
}

const lang::lexer::TokenSet lang::parser::firstset::eol = {
    lexer::TokenType::sc,
    lexer::TokenType::nl,
};

void lang::parser::Parser::parse_newlines() {
  while(expect(lexer::TokenType::nl)) consume();
}

static const lang::lexer::TokenSet numerical_types = {
    lang::lexer::TokenType::uint8,
    lang::lexer::TokenType::int8,
    lang::lexer::TokenType::uint16,
    lang::lexer::TokenType::int16,
    lang::lexer::TokenType::uint32,
    lang::lexer::TokenType::int32,
    lang::lexer::TokenType::uint64,
    lang::lexer::TokenType::int64,
    lang::lexer::TokenType::float32,
    lang::lexer::TokenType::float64,
};

// map of static type names to types
static const std::unordered_map<lang::lexer::TokenType, lang::ast::type::Node*> static_type_map = {
    {lang::lexer::TokenType::uint8, &lang::ast::type::uint8},
    {lang::lexer::TokenType::int8, &lang::ast::type::int8},
    {lang::lexer::TokenType::uint16, &lang::ast::type::uint16},
    {lang::lexer::TokenType::int16, &lang::ast::type::int16},
    {lang::lexer::TokenType::uint32, &lang::ast::type::uint32},
    {lang::lexer::TokenType::int32, &lang::ast::type::int32},
    {lang::lexer::TokenType::uint64, &lang::ast::type::uint64},
    {lang::lexer::TokenType::int64, &lang::ast::type::int64},
    {lang::lexer::TokenType::float32, &lang::ast::type::float32},
    {lang::lexer::TokenType::float64, &lang::ast::type::float64},
    {lang::lexer::TokenType::boolean, &lang::ast::type::boolean},
};

const lang::lexer::TokenSet lang::parser::firstset::number{lexer::TokenType::int_lit, lexer::TokenType::float_lit};
const lang::lexer::TokenSet lang::parser::firstset::boolean{lexer::TokenType::false_kw, lexer::TokenType::true_kw};
const lang::lexer::TokenSet lang::parser::firstset::literal = lexer::merge_sets({number, boolean});

std::unique_ptr<lang::ast::expr::LiteralNode> lang::parser::Parser::parse_literal() {
  // Boolean literal?
  if (expect(firstset::boolean)) {
    lexer::Token token = consume();
    token.value = token.type == lexer::TokenType::true_kw ? conf::bools::true_value : conf::bools::false_value;
    return std::make_unique<ast::expr::LiteralNode>(token, ast::type::boolean);
  }

  // assume token is float_lit or int_lit
  lexer::Token token = consume();
  ast::type::Node* type_node;

  // check if type is explicitly suffixed, otherwise set to default int/float types
  if (expect(numerical_types)) {
    const lexer::Token type_token = consume();
    type_node = static_type_map.at(type_token.type);
    token.parse_numerical(type_token.type); // TODO check if error?
  } else if (token.type == lexer::TokenType::float_lit) {
    type_node = &ast::type::float32;
    token.parse_numerical(lexer::TokenType::float32);
  } else {
    type_node = &ast::type::int32;
    token.parse_numerical(lexer::TokenType::int32);
  }

  return std::make_unique<ast::expr::LiteralNode>(token, *type_node);
}

const lang::lexer::TokenSet lang::parser::firstset::term = lexer::merge_sets({
    firstset::literal,
    {lexer::TokenType::ident, lexer::TokenType::lpar}
});

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::parse_term() {
  if (expect(firstset::literal)) {
    return parse_literal();
  } else if (expect(lexer::TokenType::ident)) { // identifier
    return std::make_unique<ast::expr::SymbolReferenceNode>(consume());
  } else if (expect(lexer::TokenType::lpar)) { // bracketed expression
    consume();
    auto expr = _parse_expression(0);
    // ensure we have a closing bracket, or propagate error
    if (is_error() || !expect_or_error(lexer::TokenType::rpar)) return nullptr;
    consume(); // ')'
    return expr;
  } else { // unknown, but should've been caught already
    return nullptr;
  }
}

const lang::lexer::TokenSet lang::parser::firstset::expression = lexer::merge_sets({
    firstset::term,
    {lexer::TokenType::op}
});

std::unique_ptr<lang::ast::ExprNode> lang::parser::Parser::parse_expression(int precedence) {
  auto expr = _parse_expression(precedence);
  return is_error() ? nullptr : std::make_unique<ast::ExprNode>(std::move(expr));
}

std::unique_ptr<lang::ast::expr::Node> lang::parser::Parser::_parse_expression(int precedence) {
  std::unique_ptr<ast::expr::Node> expr;

  // check if we have a unary operator, or just a term
  if (expect(lexer::TokenType::op)) {
    lexer::Token op_token = consume();
    // check if there is a term following this
    if (!expect_or_error(firstset::term)) return nullptr;
    std::unique_ptr<ast::expr::Node> term = parse_term();
    // propagate error if necessary
    if (is_error()) return nullptr;
    // wrap term in the unary operator
    return std::make_unique<ast::expr::UnaryOperatorNode>(op_token, std::move(term));
  } else if (expect(lexer::TokenType::lpar) && expect(firstset::type, 1)) { // check if **special** cast operator
    lexer::Token op_token = consume();
    auto type = parse_type();
    if (is_error() || !type) return nullptr;
    // expect closing bracket after type
    if (!expect_or_error(lexer::TokenType::rpar)) return nullptr;
    consume();
    // check if there is a term following this
    if (!expect_or_error(firstset::term)) return nullptr;
    std::unique_ptr<ast::expr::Node> term = parse_term();
    // propagate error if necessary
    if (is_error()) return nullptr;
    // wrap term in the unary operator
    return std::make_unique<ast::expr::CastOperatorNode>(op_token, *type, std::move(term));
  } else if (expect_or_error(firstset::term)) {
    expr = parse_term();
    if (is_error()) return nullptr;
  } else {
    return nullptr;
  }

  // any further operators are treated as binary
  while (expect(lexer::TokenType::op)) {
    const lexer::Token op_token = peek();
    auto& op_info = ops::builtin_binary.contains(op_token.image)
        ? ops::builtin_binary.at(op_token.image)
        : ops::generic;

    // exit if lower precedence
    if (precedence >= op_info.precedence) {
      break;
    }

    // parse RHS of expression, supplying the operator's precedence as a new baseline
    consume();
    if (!expect_or_error(firstset::expression)) return nullptr;
    int new_precedence = op_info.precedence;
    if (op_info.right_associative) new_precedence--;
    auto rest = _parse_expression(new_precedence);
    if (is_error()) return nullptr;

    // wrap both sides in a binary operator
    expr = std::make_unique<ast::expr::BinaryOperatorNode>(op_token, std::move(expr), std::move(rest));
  }

  return expr;
}

const lang::lexer::TokenSet lang::parser::firstset::type = lexer::merge_sets({
    numerical_types,
    {lexer::TokenType::boolean}
});

const lang::ast::type::Node* lang::parser::Parser::parse_type() {
  if (auto it = static_type_map.find(peek().type); it != static_type_map.end()) {
    consume();
    return it->second;
  }

  return nullptr;
}

std::unique_ptr<lang::ast::SymbolDeclarationNode> lang::parser::Parser::parse_name_type_pair() {
  // name
  const lexer::Token name = consume();

  // ':'
  if (!expect_or_error(lexer::TokenType::colon)) return nullptr;
  consume();

  // type
  if (!expect_or_error(firstset::type)) return nullptr;
  const ast::type::Node* type = parse_type();
  if (is_error() || !type) return nullptr;

  return std::make_unique<ast::SymbolDeclarationNode>(name, *type);
}

void lang::parser::Parser::parse_var_decl(ast::ContainerNode& container) {
  // consume 'let' or 'const'
  const auto kw_token = consume();
  bool is_const = false;
  const auto category = (is_const = kw_token.type == lexer::TokenType::const_kw)
      ? ast::SymbolDeclarationNode::Constant
      : ast::SymbolDeclarationNode::Variable;

  while (true) {
    // name
    if (!expect_or_error(lexer::TokenType::ident)) return;
    const lexer::Token& name = consume();

    // are we attaching a type? if not, attempt to deduce it
    std::optional<std::reference_wrapper<const lang::ast::type::Node>> type;
    if (expect(lexer::TokenType::colon)) {
      consume();
      if (!expect_or_error(firstset::type)) return;
      auto type_ptr = parse_type();
      if (is_error() || !type_ptr) return;
      type = *type_ptr;
    }

    // if assignment, expect expression
    std::optional<std::unique_ptr<ast::ExprNode>> expr;
    if (expect(lexer::BasicToken(lexer::TokenType::op, "="))) {
      consume();
      expr = parse_expression();
      if (is_error()) return;
    }

    // create and add declaration node
    auto decl = std::make_unique<ast::SymbolDeclarationNode>(name, type, std::move(expr));
    decl->set_category(category);
    container.add(std::move(decl));

    // if comma, continue
    if (!expect(lexer::TokenType::comma)) break;
    consume();
  }
}

std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> lang::parser::Parser::parse_arg_list() {
  // '('
  const lexer::Token lpar = consume();

  std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> args;
  while (true) {
    // parse `name: type` pair
    if (!expect_or_error(lexer::TokenType::ident)) return {};
    args.push_back(parse_name_type_pair());
    args.back()->set_category(ast::SymbolDeclarationNode::Argument);
    if (is_error()) return {};

    // if comma, continue
    if (!expect(lexer::TokenType::comma)) break;
    consume();
  }

  // ')'
  if (!expect_or_error(lexer::TokenType::rpar)) {
    auto msg = lpar.generate_message(message::Note);
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

  // create function type
  std::deque<std::reference_wrapper<const ast::type::Node>> param_types;
  for (auto& param : params) param_types.push_back(param->type());
  auto& type = ast::type::FunctionNode::create(param_types, returns);

  // construct function type and node
  return std::make_unique<ast::FunctionNode>(name, type, std::move(params), std::move(body));
}

std::unique_ptr<lang::ast::ReturnNode> lang::parser::Parser::parse_return() {
  // 'return'
  const lexer::Token token = consume();

  // is there an expression?
  std::optional<std::unique_ptr<ast::ExprNode>> expr;
  if (expect(firstset::expression)) {
    expr = parse_expression();
    if (is_error()) return nullptr;
  }

  return std::make_unique<ast::ReturnNode>(std::move(token), std::move(expr));
}

const lang::lexer::TokenSet lang::parser::firstset::line = lexer::merge_sets({
   {lexer::TokenType::const_kw, lexer::TokenType::func, lexer::TokenType::let, lexer::TokenType::namespace_kw, lexer::TokenType::return_kw},
   firstset::expression,
});

void lang::parser::Parser::parse_line(lang::ast::BlockNode& block) {
  static bool was_last_an_expression = false;

  if (expect(firstset::expression)) {
    // one expression cannot immediately follow another
    if (was_last_an_expression) {
      add_message(peek().generate_syntax_error(firstset::eol));
      return;
    }
    was_last_an_expression = true;
    block.add(parse_expression());
    return;
  }
  was_last_an_expression = false;

  if (expect(lexer::TokenType::func)) {
    block.add(parse_func());
    return;
  }

  if (expect({lexer::TokenType::let, lexer::TokenType::const_kw})) {
    parse_var_decl(block);
    return;
  }

  if (expect(lexer::TokenType::namespace_kw)) {
    block.add(parse_namespace());
    return;
  }

  if (expect(lexer::TokenType::return_kw)) {
    block.add(parse_return());
    return;
  }
}

std::unique_ptr<lang::ast::BlockNode> lang::parser::Parser::parse_block() {
  // '{'
  const lexer::Token lbrace = consume();

  auto block = std::make_unique<ast::BlockNode>(lbrace);
  while (true) {
    // remove newlines
    while (expect(firstset::eol)) consume();
    if (expect(lexer::TokenType::rbrace)) break;

    // parse line or '}'
    if (!expect_or_error(lexer::merge_sets({firstset::line, {lexer::TokenType::rbrace}}))) {
      auto msg = lbrace.generate_message(message::Note);
      msg->get() << "block started here";
      add_message(std::move(msg));
      return nullptr;
    }

    if (expect(lexer::TokenType::rbrace)) {
      consume();
      break;
    }

    parse_line(*block);
    if (is_error()) return nullptr;
  }

  consume();
  return block;
}

std::unique_ptr<lang::ast::NamespaceNode> lang::parser::Parser::parse_namespace() {
  // 'namespace'
  consume();

  // name
  if (!expect_or_error(lexer::TokenType::ident)) return nullptr;
  const lexer::Token name = consume();

  // body (block)
  auto block = parse_block();
  if (is_error()) return nullptr;

  return std::make_unique<ast::NamespaceNode>(std::move(name), std::move(block));
}

const lang::lexer::TokenSet lang::parser::firstset::top_level_line = lexer::merge_sets({
    {lexer::TokenType::const_kw,
     lexer::TokenType::func,
     lexer::TokenType::let,
     lexer::TokenType::namespace_kw},
     firstset::expression,
});

void lang::parser::Parser::parse_top_level_line(ast::ProgramNode& program) {
  static bool was_last_an_expression = false;

  if (expect(firstset::expression)) {
    // one expression cannot immediately follow another
    if (was_last_an_expression) {
      add_message(peek().generate_syntax_error(firstset::eol));
      return;
    }
    program.add(parse_expression());
    was_last_an_expression = true;
    return;
  }
  was_last_an_expression = false;

  if (expect(lexer::TokenType::func)) {
    program.add(parse_func());
    return;
  }

  if (expect({lexer::TokenType::let, lexer::TokenType::const_kw})) {
    parse_var_decl(program);
    return;
  }

  if (expect(lexer::TokenType::namespace_kw)) {
    program.add(parse_namespace());
    return;
  }
}

std::unique_ptr<lang::ast::ProgramNode> lang::parser::Parser::parse() {
  auto program = std::make_unique<ast::ProgramNode>(peek());

  while (true) {
    // remove newlines
    while (expect(firstset::eol)) consume();
    if (expect(lexer::TokenType::eof)) break;

    // parse line
    if (!expect_or_error(firstset::top_level_line)) return nullptr;
    parse_top_level_line(*program);
    if (is_error()) return nullptr;
  }

  return program;
}
