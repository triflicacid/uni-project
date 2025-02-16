#include <iostream>
#include <map>
#include <ranges>
#include "parser.hpp"
#include "ast/block.hpp"
#include "ast/types/float.hpp"
#include "ast/types/int.hpp"
#include "ast/expr/operator.hpp"
#include "ast/expr/symbol_reference.hpp"
#include "util.hpp"
#include "ast/program.hpp"
#include "operators/info.hpp"
#include "ast/types/bool.hpp"
#include "ast/types/pointer.hpp"
#include "config.hpp"
#include "ast/expr/unit.hpp"

void lang::parser::Parser::read_tokens(unsigned int n) {
  for (unsigned int i = 0; i < n; i++) {
    buffer_.emplace_back(lexer_.next());
    if (buffer_.back().is_eof()) break;
  }
}

void lang::parser::Parser::add_message(std::unique_ptr<message::BasicMessage> m) {
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

bool lang::parser::Parser::expect(const lexer::TokenSet& tokens, unsigned int n) {
  auto& next = peek(n);
  for (auto& token : tokens) {
    if (token == next) return true;
  }
  return false;
}

bool lang::parser::Parser::expect(const lang::lexer::BasicToken& token, unsigned int n) {
  return peek(n) == token;
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

bool lang::parser::Parser::expect_or_error(const lexer::TokenSet& tokens) {
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
  prev_ = buffer_.front();
  buffer_.pop_front();
  return prev_;
}

// '*' token
static const lang::lexer::BasicToken star(lang::lexer::TokenType::op, "*");

// '->' token
static const lang::lexer::BasicToken arrow(lang::lexer::TokenType::op, "->");

// '!' token
static const lang::lexer::BasicToken bang(lang::lexer::TokenType::op, "!");

const lang::lexer::TokenSet lang::parser::firstset::eol{
    lexer::BasicToken{lexer::TokenType::sc}
};

static const lang::lexer::TokenTypeSet numerical_types = {
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

const lang::lexer::TokenSet lang::parser::firstset::number{
  lexer::BasicToken(lexer::TokenType::int_lit),
  lexer::BasicToken(lexer::TokenType::float_lit)
};
const lang::lexer::TokenSet lang::parser::firstset::boolean{
  lexer::BasicToken(lexer::TokenType::false_kw),
  lexer::BasicToken(lexer::TokenType::true_kw)
};
const lang::lexer::TokenSet lang::parser::firstset::literal = lexer::merge_sets({number, boolean});

std::unique_ptr<lang::ast::LiteralNode> lang::parser::Parser::parse_literal() {
  // Boolean literal?
  if (expect(firstset::boolean)) {
    lexer::Token token = consume();
    auto& lit = memory::Literal::get_boolean(token.type == lexer::TokenType::true_kw);
    return std::make_unique<ast::LiteralNode>(token, lit);
  }

  // assume token is float_lit or int_lit
  lexer::Token token = consume();
  auto node = std::make_unique<ast::LiteralNode>(token);

  // if suffixed, this gives us more of a type hint
  if (expect(numerical_types)) {
    const lexer::Token type_token = consume();
    node->type_hint(*static_type_map.at(type_token.type));
  }

  return node;
}

const lang::lexer::TokenSet lang::parser::firstset::term = lexer::merge_sets({
    firstset::literal,
    {
      lexer::BasicToken(lexer::TokenType::ident),
      lexer::BasicToken(lexer::TokenType::if_kw),
      lexer::BasicToken(lexer::TokenType::lpar),
      lexer::BasicToken(lexer::TokenType::lbrace),
      lexer::BasicToken(lexer::TokenType::null_kw),
    }
});

std::unique_ptr<lang::ast::Node> lang::parser::Parser::parse_term() {
  if (expect(firstset::literal)) {
    return parse_literal();
  }

  if (expect(lexer::TokenType::ident)) { // identifier
    const lexer::Token token = consume();
    return std::make_unique<ast::SymbolReferenceNode>(token, token.image);
  }

  if (expect(lexer::TokenType::if_kw)) { // if statement
    return parse_if_statement();
  }

  if (expect(lexer::TokenType::lpar)) { // bracketed expression
    const lexer::Token token_start = consume();
    // if there's an immediate ')', then this refers to the unit
    if (expect(lexer::TokenType::rpar)) {
      auto unit = std::make_unique<ast::UnitNode>(token_start);
      unit->token_end(consume());
      return unit;
    }
    // otherwise, try to parse an expression
    if (!expect_or_error(firstset::expression)) return nullptr;
    auto expr = parse_expression_internal(0);
    // ensure we have a closing bracket, or propagate error
    if (is_error() || !expect_or_error(lexer::TokenType::rpar)) return nullptr;
    const lexer::Token token_end = consume(); // ')'
    // include brackets in token span
    expr->token_start(token_start);
    expr->token_end(token_end);
    return expr;
  }

  if (expect(lexer::TokenType::lbrace)) { // block
    return parse_block();
  }

  if (expect(lexer::TokenType::null_kw)) { // null literal
    return std::make_unique<ast::LiteralNode>(consume());
  }

  return nullptr;
}

const lang::lexer::TokenSet lang::parser::firstset::op = lexer::convert_set({
    lexer::TokenType::op,
    lexer::TokenType::registerof_kw,
    lexer::TokenType::sizeof_kw,
});

const lang::lexer::TokenSet lang::parser::firstset::expression = lexer::merge_sets({
    firstset::term,
    firstset::op
});

bool lang::parser::Parser::check_semicolon_after_expression(bool generate_messages) {
  if (generate_messages) {
    if (expect_or_error(lexer::TokenType::sc)) {
      consume();
      return true;
    }

    // generate messages  + error --> add note
    auto msg = previous().generate_message(message::Note);
    msg->get() << "an expression must be terminated by a semicolon";
    add_message(std::move(msg));
  } else if (expect(lexer::TokenType::sc)) {
    consume();
    return true;
  }

  return false;
}

std::unique_ptr<lang::ast::Node> lang::parser::Parser::parse_expression(ExprExpectSC expect_sc, int precedence) {
  auto expr = parse_expression_internal(precedence);
  if (is_error()) return nullptr;

  if (expect_sc == ExprExpectSC::No) {
    return expr;
  }

  bool found_sc = check_semicolon_after_expression(expect_sc == ExprExpectSC::Yes);
  if (!found_sc) expr_last_ = &expr->token_end();

  if (expect_sc == ExprExpectSC::Yes) {
    if (!found_sc) return nullptr;
    return expr;
  }

  // expect_sc == ExprExpectSC::Maybe
  expect_block_end = !found_sc;
  return expr;
}

std::unique_ptr<lang::ast::Node> lang::parser::Parser::parse_expression_internal(int precedence) {
  std::unique_ptr<ast::Node> expr;

  if (expect(firstset::op)) { // unary operator
    const lexer::Token op_token = consume();

    // if `sizeof`, may be preceded by a type instead
    if (op_token.type == lexer::TokenType::sizeof_kw && expect(firstset::type)) {
      auto type = parse_type();
      if (is_error() || !type) return nullptr;
      expr = std::make_unique<ast::SizeOfOperatorNode>(op_token, op_token, *type);
    } else {
      // must be followed by an expression
      if (!expect_or_error(firstset::expression)) return nullptr;
      auto& info = ops::builtin_unary.contains(op_token.image)
                   ? ops::builtin_unary.at(op_token.image)
                   : ops::generic_unary;

      expr = parse_expression_internal(info.precedence);
      if (is_error()) return nullptr;
      // wrap and continue
      expr = ast::OperatorNode::unary(op_token, std::move(expr));
    }
  }
  else if (expect(lexer::TokenType::lpar) && expect(numerical_types, 1)) { // primitive cast
    const lexer::Token op_token = consume();

    // expect type `)`
    auto type = parse_type();
    if (is_error() || !expect_or_error(lexer::TokenType::rpar)) return nullptr;
    consume();

    // followed by an expression
    if (!expect_or_error(firstset::expression)) return nullptr;
    auto& info = ops::builtin_unary.at("(type)");
    expr = parse_expression_internal(info.precedence);
    if (is_error()) return nullptr;

    // wrap and continue
    expr = std::make_unique<ast::CStyleCastOperatorNode>(op_token, *type, std::move(expr));
  }
  else if (expect(firstset::term)) { // term
    expr = parse_term();
    if (is_error()) return nullptr;
  } else { // uncaught error
    return nullptr;
  }

  // sequence of binary operators
  while (true) {
    // given token, lookup operator info which tells us how to continue parsing
    const lexer::Token op_token = peek();
    const ops::OperatorInfo* info;

    if (expect(lexer::TokenType::op)) {
      info = ops::builtin_binary.contains(op_token.image)
             ? &ops::builtin_binary.at(op_token.image)
             : &ops::generic_binary;
      // exit if lower precedence
      if (precedence >= info->precedence) break;
      consume();
    } else if (expect(lexer::TokenType::lpar)) { // '(' --> function call
      info = &ops::builtin_binary.at("()");
      // exit if lower precedence - some things are tighter than a call
      if (precedence >= info->precedence) break;
      // parse call and make new LHS
      expr = parse_function_call(std::move(expr));
      if (is_error()) return nullptr;
      continue; // continue to next operator
    } else if (expect(lexer::TokenType::lsquare)) { // '[' --> subscript
      info = &ops::builtin_binary.at("[]");
      const lexer::Token lsquare = consume();
      // exit if lower precedence - some things are tighter than a call
      if (precedence >= info->precedence) break;
      // parse inner expression
      if (!expect_or_error(firstset::expression)) return nullptr;
      auto inner = parse_expression_internal(0);
      if (is_error()) return nullptr;
      // expect ']'
      if (!expect_or_error(lexer::TokenType::rsquare)) {
        auto msg = lsquare.generate_message(message::Note);
        msg->get() << "subscript opened here";
        add_message(std::move(msg));
        return nullptr;
      }
      consume();
      // construct node and continue
      lexer::Token token_start = expr->token_start();
      lexer::Token symbol = op_token;
      symbol.image = "[]";
      expr = ast::OperatorNode::binary(token_start, symbol, std::move(expr), std::move(inner));
      expr->token_end(previous());
      continue; // continue to next operator
    } else if (expect(lexer::TokenType::as_kw)) { // 'as' --> cast
      info = &ops::builtin_binary.at("as");
      // exit if lower precedence - some things are tighter than a 'as'
      if (precedence >= info->precedence) break;
      consume();
      // are we sudo or not?
      bool is_sudo = false;
      if (expect(bang)) {
        is_sudo = true;
        consume();
      }
      // expect type
      if (!expect_or_error(firstset::type)) return nullptr;
      auto type = parse_type();
      if (is_error() || !type) return nullptr;
      // construct node and continue to next operation
      const lexer::Token token_start = expr->token_start();
      expr = std::make_unique<ast::CastOperatorNode>(token_start, *type, std::move(expr), is_sudo);
      continue;
    } else {
        break;
    }

    // parse RHS of expression, supplying the operator's precedence as a new baseline
    if (!expect_or_error(firstset::expression)) return nullptr;
    int new_precedence = info->precedence;
    if (info->right_associative) new_precedence--;
    auto rest = parse_expression_internal(new_precedence);
    if (is_error()) return nullptr;

    // wrap both sides in a binary operator and continue
    const lexer::Token token_start = expr->token_start();
    expr = ast::OperatorNode::binary(token_start, op_token, std::move(expr), std::move(rest));
  }

  return expr;
}

const lang::lexer::TokenSet lang::parser::firstset::type = lexer::merge_sets({
    lexer::convert_set(numerical_types),
    {
      lexer::BasicToken(lexer::TokenType::boolean),
      lexer::BasicToken(lexer::TokenType::lpar),
      star
    }
});

const lang::ast::type::Node* lang::parser::Parser::parse_type() {
  // is this a static type match?
  if (auto it = static_type_map.find(peek().type); it != static_type_map.end()) {
    consume();
    return it->second;
  }

  // pointer?
  if (expect(star)) {
    consume();
    if (!expect_or_error(firstset::type)) return nullptr;
    auto inner = parse_type();
    if (is_error() || !inner) return nullptr;
    return &ast::type::PointerNode::get(*inner);
  }

  // unit or function type
  if (expect(lexer::TokenType::lpar)) {
    const lexer::Token lpar = consume();
    std::deque<std::reference_wrapper<const ast::type::Node>> arg_types; // store function argument types

    // if close straight away: either no args, or unit
    if (expect(lexer::TokenType::rpar)) {
      consume();

      // arrow for function type, if not, return '()'
      if (!expect(arrow)) {
        return &ast::type::unit;
      }
    } else {
      // parse type list
      while (true) {
        if (!expect_or_error(firstset::type)) return nullptr;
        auto arg_type = parse_type();
        if (is_error() || !arg_type) return nullptr;
        arg_types.push_back(*arg_type);

        // comma to continue, exit otherwise
        if (!expect(lexer::TokenType::comma)) break;
        consume();
      }

      // expect closing bracket now
      if (!expect_or_error(lexer::TokenType::rpar)) {
        auto msg = lpar.generate_message(message::Note);
        msg->get() << "while parsing function type: argument list opened here";
        add_message(std::move(msg));
        return nullptr;
      }
      consume();
    }

    // expect arrow '->' - return type must be explicit
    if (!expect_or_error(arrow)) {
      auto msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "while parsing function type: return type must be explicit";
      add_message(std::move(msg));
      return nullptr;
    }
    consume();

    // parse return type
    if (!expect_or_error(firstset::type)) return nullptr;
    auto return_type = parse_type();
    if (is_error() || !return_type) return nullptr;

    // return function type
    return &ast::type::FunctionNode::create(arg_types, *return_type);
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

  auto node = std::make_unique<ast::SymbolDeclarationNode>(name, name, *type);
  node->token_end(previous());
  return node;
}

void lang::parser::Parser::parse_var_decl(ast::ContainerNode& container) {
  // consume 'let' or 'const'
  const auto kw_token = consume();
  const auto category = kw_token.type == lexer::TokenType::const_kw
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
    std::optional<std::unique_ptr<ast::Node>> expr;
    if (expect(lexer::BasicToken(lexer::TokenType::op, "="))) {
      consume();
      expr = parse_expression(ExprExpectSC::No);
      if (is_error()) return;
    }

    // create and add declaration node
    auto decl = std::make_unique<ast::SymbolDeclarationNode>(name, name, type, std::move(expr));
    decl->set_category(category);
    decl->token_end(previous());
    container.add(std::move(decl));

    // if comma, continue
    if (!expect(lexer::TokenType::comma)) break;
    consume();
  }

  if (!check_semicolon_after_expression()) return;
}

std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> lang::parser::Parser::parse_param_list() {
  // '('
  const lexer::Token lpar = consume();

  std::deque<std::unique_ptr<lang::ast::SymbolDeclarationNode>> args;
  if (!expect(lexer::TokenType::rpar)) {
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

std::unique_ptr<lang::ast::FunctionCallOperatorNode> lang::parser::Parser::parse_function_call(std::unique_ptr<ast::Node> subject) {
  // '('
  const lexer::Token lpar = consume();

  std::deque<std::unique_ptr<ast::Node>> args;
  if (!expect(lexer::TokenType::rpar)) {
    while (true) {
      // parse argument expression
      if (!expect_or_error(firstset::expression)) return nullptr;
      args.push_back(parse_expression(ExprExpectSC::No));
      if (is_error()) return nullptr;

      // if comma, continue
      if (!expect(lexer::TokenType::comma)) break;
      consume();
    }

    // ')'
    if (!expect_or_error(lexer::TokenType::rpar)) {
      auto msg = lpar.generate_message(message::Note);
      msg->get() << "parenthesis opened here";
      add_message(std::move(msg));
      return nullptr;
    }
  }
  const lexer::Token rpar = consume();

  // construct function call node
  const lexer::Token token_start = subject->token_start();
  auto node = std::make_unique<ast::FunctionCallOperatorNode>(token_start, lpar, std::move(subject), std::move(args));
  node->token_end(rpar);
  return node;
}

std::unique_ptr<lang::ast::FunctionBaseNode> lang::parser::Parser::parse_function_tail(lang::lexer::Token token_start, lang::lexer::Token name, const std::function<std::unique_ptr<ast::FunctionBaseNode>(FunctionTailContent)>& create) {
  // parse any supplied arguments
  std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> params;
  if (expect(lexer::TokenType::lpar)) {
    params = parse_param_list();
    if (is_error()) return nullptr;
  }

  // parse an optional return type
  std::reference_wrapper<const ast::type::Node> returns = ast::type::unit;
  if (expect(arrow)) {
    consume();
    if (!expect_or_error(firstset::type)) return nullptr;
    auto type = parse_type();
    if (is_error() || !type) return nullptr;
    returns = *type;
  }

  // parse function body (optional)
  if (!expect_or_error(lexer::TokenTypeSet{lexer::TokenType::sc, lexer::TokenType::lbrace})) return nullptr;
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
  FunctionTailContent content{
    .token = token_start,
    .name = name,
    .type = type,
    .params = std::move(params),
    .body = std::move(body)
  };
  auto node = create(std::move(content));
  node->token_end(previous());
  return node;
}

std::unique_ptr<lang::ast::FunctionBaseNode> lang::parser::Parser::parse_func() {
  // 'func'
  const lexer::Token token_start = consume();

  // name
  if (!expect_or_error(lexer::TokenType::ident)) return nullptr;
  const lexer::Token name = consume();

  // parse tail & create
  return parse_function_tail(token_start, name, [](FunctionTailContent content) {
    return std::make_unique<ast::FunctionNode>(
        content.token,
        content.name,
        content.type,
        std::move(content.params),
        std::move(content.body)
      );
  });
}

std::unique_ptr<lang::ast::FunctionBaseNode> lang::parser::Parser::parse_operator_definition() {
  // tokens we might expect after 'operator'
  static const lexer::TokenTypeSet valid_names = {
    lexer::TokenType::op,
    lexer::TokenType::lpar,
    lexer::TokenType::lsquare,
  };

  // 'operator'
  const lexer::Token token_start = consume();

  // expect an operator/valid name
  if (!expect_or_error(valid_names)) return nullptr;
  lexer::Token name = peek();

  if (expect(lexer::TokenType::lpar)) {  // if '(', expect '()'
    consume();
    if (!expect_or_error(lexer::TokenType::rpar)) {
      // generate help message about expecting '()'
      auto msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "expected '()' operator here";
      add_message(std::move(msg));
      return nullptr;
    }
    consume();
    name.image = "()"; // lil' hack to ensure image is OK
  } else if (expect(lexer::TokenType::lsquare)) {  // if '[', expect '[]'
      consume();
      if (!expect_or_error(lexer::TokenType::rsquare)) {
        // generate help message about expecting '()'
        auto msg = std::make_unique<message::BasicMessage>(message::Note);
        msg->get() << "expected '[]' operator here";
        add_message(std::move(msg));
        return nullptr;
      }
      consume();
      name.image = "[]"; // hack to ensure image is OK
    } else {
    consume();
  }

  // parse tail & create
  return parse_function_tail(token_start, name, [](FunctionTailContent content) {
    return std::make_unique<ast::OperatorDefinitionNode>(
        content.token,
        content.name,
        content.type,
        std::move(content.params),
        std::move(content.body)
    );
  });
}

std::unique_ptr<lang::ast::ReturnNode> lang::parser::Parser::parse_return() {
  // 'return'
  const lexer::Token token = consume();

  // is there an expression?
  std::optional<std::unique_ptr<ast::Node>> expr;
  if (expect(firstset::expression)) {
    expr = parse_expression(ExprExpectSC::Yes);
    if (is_error()) return nullptr;
  }

  auto node = std::make_unique<ast::ReturnNode>(std::move(token), std::move(expr));
  if (expr.has_value()) node->token_end(previous());
  return node;
}

const lang::lexer::TokenSet lang::parser::firstset::line = lexer::merge_sets({
   {
     lexer::BasicToken(lexer::TokenType::const_kw),
     lexer::BasicToken(lexer::TokenType::func),
     lexer::BasicToken(lexer::TokenType::if_kw),
     lexer::BasicToken(lexer::TokenType::let),
     lexer::BasicToken(lexer::TokenType::namespace_kw),
     lexer::BasicToken(lexer::TokenType::return_kw),
   },
   firstset::expression,
});

std::unique_ptr<lang::ast::IfElseNode> lang::parser::Parser::parse_if_statement() {
  // 'if'
  const lexer::Token token_start = consume();

  // expect expression
  if (!expect_or_error(firstset::expression)) return nullptr;
  auto guard = parse_expression(ExprExpectSC::No);

  // expect 'then' body
  auto then_body = parse_block_or_line(false);
  if (is_error()) return nullptr;

  // 'else' ?
  std::optional<std::unique_ptr<ast::Node>> else_body;
  if (expect(lexer::TokenType::else_kw)) {
    consume();
    else_body = parse_block_or_line(false);
    if (is_error()) return nullptr;
  }

  return std::make_unique<ast::IfElseNode>(token_start, std::move(guard), std::move(then_body), std::move(else_body));
}

std::unique_ptr<lang::ast::BlockNode> lang::parser::Parser::parse_block_or_line(bool in_top_level) {
  if (expect(lexer::TokenType::lbrace)) {
    return parse_block();
  }

  auto block = std::make_unique<ast::BlockNode>(peek());
  if (!parse_line(*block, in_top_level) || is_error()) return nullptr;
  return block;
}

void lang::parser::Parser::parse_line(lang::ast::BlockNode& block) {
  if (expect(firstset::expression)) {
    block.add(parse_expression(ExprExpectSC::Maybe));
    return;
  }

  if (expect(lexer::TokenType::func)) {
    block.add(parse_func());
    return;
  }

  if (expect(lexer::TokenTypeSet{lexer::TokenType::let, lexer::TokenType::const_kw})) {
    parse_var_decl(block);
    return;
  }

  if (expect(lexer::TokenType::if_kw)) {
    block.add(parse_if_statement());
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

bool lang::parser::Parser::parse_line(ast::BlockNode& block, bool top_level) {
  if (top_level) {
    if (!expect_or_error(firstset::top_level_line)) return false;
    parse_top_level_line(block);
    return !is_error();
  }

  if (!expect_or_error(firstset::line)) return false;
  parse_line(block);
  return !is_error();
}

std::unique_ptr<lang::ast::BlockNode> lang::parser::Parser::parse_block() {
  static const auto line_or_brace = lexer::merge_sets({firstset::line, {lexer::BasicToken(lexer::TokenType::rbrace)}});

  // '{'
  const lexer::Token lbrace = consume();
  expect_block_end = false;

  auto block = std::make_unique<ast::BlockNode>(lbrace);
  while (true) {
    while (expect(firstset::eol)) consume();

    // parse line or '}'
    if (expect(lexer::TokenType::rbrace)) break;
    if (!expect_or_error(line_or_brace)) {
      auto msg = lbrace.generate_message(message::Note);
      msg->get() << "block started here";
      add_message(std::move(msg));
      return nullptr;
    }

    parse_line(*block);
    if (is_error()) return nullptr;

    // expect end of block here?
    if (expect_block_end) {
      if (expect_or_error(lexer::TokenType::rbrace)) break;
      auto msg = expr_last_->generate_message(message::Note);
      msg->get() << "non-semicolon-terminated expression must be last in block";
      add_message(std::move(msg));
    }
  }

  consume();
  block->token_end(previous());

  // block returns if non-sc-terminated expr present
  if (expect_block_end) block->make_expr();

  return block;
}

std::unique_ptr<lang::ast::NamespaceNode> lang::parser::Parser::parse_namespace() {
  static const auto line_or_brace = lexer::merge_sets({firstset::top_level_line, {lexer::BasicToken(lexer::TokenType::rbrace)}});

  // 'namespace'
  const lexer::Token token_start = consume();

  // parse dot-separated names
  std::deque<lexer::Token> names;
  while (true) {
    // name
    if (!expect_or_error(lexer::TokenType::ident)) return nullptr;
    names.push_back(consume());

    // '.' to continue?
    if (!expect(lexer::BasicToken(lexer::TokenType::op, "."))) break;
    consume();
  }

  // '{'
  if (!expect_or_error(lexer::TokenType::lbrace)) return nullptr;
  const lexer::Token lbrace = consume();

  auto ns = std::make_unique<ast::NamespaceNode>(token_start, names);
  while (true) {
    // remove newlines
    while (expect(firstset::eol)) consume();
    if (expect(lexer::TokenType::rbrace)) break;

    // parse line or '}'
    if (!expect_or_error(line_or_brace)) {
      auto msg = lbrace.generate_message(message::Note);
      msg->get() << "block started here";
      add_message(std::move(msg));
      return nullptr;
    }

    if (expect(lexer::TokenType::rbrace)) {
      consume();
      break;
    }

    parse_top_level_line(*ns);
    if (is_error()) return nullptr;
  }

  consume();
  ns->token_end(previous());
  return ns;
}

const lang::lexer::TokenSet lang::parser::firstset::top_level_line = lexer::merge_sets({
    {
      lexer::BasicToken(lexer::TokenType::lbrace),
      lexer::BasicToken(lexer::TokenType::const_kw),
      lexer::BasicToken(lexer::TokenType::func),
      lexer::BasicToken(lexer::TokenType::if_kw),
      lexer::BasicToken(lexer::TokenType::let),
      lexer::BasicToken(lexer::TokenType::namespace_kw),
      lexer::BasicToken(lexer::TokenType::operator_kw),
    },
    firstset::expression,
});

void lang::parser::Parser::parse_top_level_line(ast::ContainerNode& container) {
  if (expect(lexer::TokenType::lbrace)) {
    container.add(parse_block());
    return;
  }

  if (expect(lexer::TokenType::func)) {
    container.add(parse_func());
    return;
  }

  if (expect(lexer::TokenTypeSet{lexer::TokenType::let, lexer::TokenType::const_kw})) {
    parse_var_decl(container);
    return;
  }

  if (expect(lexer::TokenType::if_kw)) {
    container.add(parse_if_statement());
    return;
  }

  if (expect(lexer::TokenType::namespace_kw)) {
    container.add(parse_namespace());
    return;
  }

  if (expect(lexer::TokenType::operator_kw)) {
    container.add(parse_operator_definition());
    return;
  }

  if (expect(firstset::expression)) {
    container.add(parse_expression(ExprExpectSC::Yes));
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
