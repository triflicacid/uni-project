#pragma once

#include <set>
#include "lexer/lexer.hpp"
#include "messages/MessageWithSource.hpp"
#include "messages/list.hpp"
#include "ast/expr/literal.hpp"
#include "ast/symbol_declaration.hpp"
#include "ast/function/function.hpp"
#include "ast/program.hpp"
#include "ast/function/return.hpp"
#include "ast/namespace.hpp"
#include "ast/expr/operator.hpp"
#include "ast/function/operator_definition.hpp"
#include "ast/if_statement.hpp"

namespace lang::parser {
  class Parser {
    lexer::Lexer& lexer_;
    std::deque<lexer::Token> buffer_; // store upcoming Tokens
    lexer::Token prev_; // store previous token
    message::List* messages_;

    bool expect_block_end; // tracks if we're at the last expression in a block
    const lexer::Token* expr_last_; // points to the last token of an expression

    // read at most n tokens from the lexer, stop on eof
    void read_tokens(unsigned int n);

    void add_message(std::unique_ptr<message::BasicMessage> m);

    // parse an expression recursively
    std::unique_ptr<ast::Node> parse_expression_internal(int precedence);

    // content from parsing a function tail, used in ::parse_function_tail callback
    struct FunctionTailContent {
      lexer::Token token;
      lexer::Token name;
      const ast::type::FunctionNode& type;
      std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> params;
      std::optional<std::unique_ptr<ast::BlockNode>> body;
    };

    // given start token and name, parse parameter list, return type, and body
    // callback is used to create class instance
    std::unique_ptr<ast::FunctionBaseNode> parse_function_tail(lexer::Token token_start, lexer::Token name, const std::function<std::unique_ptr<ast::FunctionBaseNode>(FunctionTailContent)>& create);

  public:
    explicit Parser(lexer::Lexer& lexer) : lexer_(lexer), prev_(lexer::Token::invalid(lexer.stream())) {}

    const lexer::Lexer& lexer() const { return lexer_; }

    message::List* messages() { return messages_; }

    void messages(message::List* messages) { messages_ = messages; }

    // test if there was an error
    bool is_error() const;

    // get the previous token
    const lexer::Token& previous() const { return prev_; }

    // peek n tokens into the future, default gets the next token
    const lexer::Token& peek(unsigned int n = 0);

    // same as peek(), but checks if the token type matches the given types
    bool expect(const lexer::TokenTypeSet& types, unsigned int n = 0);
    bool expect(lexer::TokenType type, unsigned int n = 0);

    // same as expect(), but matches token type and image
    bool expect(const lexer::TokenSet& tokens, unsigned int n = 0);
    bool expect(const lexer::BasicToken& token, unsigned int n = 0);

    // same as expect(types), but adds syntax error if failure
    bool expect_or_error(const lexer::TokenTypeSet& types);
    bool expect_or_error(lexer::TokenType type);
    bool expect_or_error(const lexer::TokenSet& tokens);
    bool expect_or_error(const lexer::BasicToken& token);

    // consume the current token and return it (this becomes the previous token)
    lexer::Token consume();

    // parse a numeric/boolean literal
    std::unique_ptr<ast::LiteralNode> parse_literal();

    // parse a term - a number, symbol reference, bracketed expression etc.
    std::unique_ptr<ast::Node> parse_term();

    // parse a type, return pointer to type, or nullptr if invalid
    const ast::type::Node* parse_type();

    // expect semicolon to follow an expression
    // check for SC and return if found
    // note this consumes the semicolon
    bool check_semicolon_after_expression(bool generate_messages = true);

    enum class ExprExpectSC {
      No, // no sc required or expected
      Maybe, // not requires but may be present (sets terminated_ property)
      Yes, // absolutely required
    };

    // parse an expression
    // argument: check if semicolon, updates `expect_block_end` flag
    std::unique_ptr<ast::Node> parse_expression(ExprExpectSC expect_sc, int precedence = 0);

    // parse a `name: type` pair as a symbol declaration
    std::unique_ptr<ast::SymbolDeclarationNode> parse_name_type_pair();

    // parse a `let ...` or `const ...` statement
    // assignments in definitions are allows, in which case a SymbolDeclarationNode will be followed by an ExprNode
    void parse_var_decl(ast::ContainerNode& container);

    // parse parameter list: (arg: type, ...)
    std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> parse_param_list();

    // parse a function call: given subject, parse argument list `(...)` and return node
    std::unique_ptr<ast::FunctionCallOperatorNode> parse_function_call(std::unique_ptr<ast::Node> subject);

    // parse a function statement
    std::unique_ptr<ast::FunctionBaseNode> parse_func();

    // parse an operator statement
    std::unique_ptr<ast::FunctionBaseNode> parse_operator_definition();

    // parse a return statement
    std::unique_ptr<ast::ReturnNode> parse_return();

    // parse an if..else.. statement
    std::unique_ptr<ast::IfStatementNode> parse_if_statement();

    // parse a block if '{' or a code line
    std::unique_ptr<ast::BlockNode> parse_block_or_line(bool in_top_level);

    // parse a code line
    void parse_line(ast::BlockNode& block);

    // parse a code line - ordinary line or top-level?
    // this checks the respective firstset, so no need for checking beforehand
    // return if success
    bool parse_line(ast::BlockNode& block, bool top_level);

    // parse a code block `{...}`
    std::unique_ptr<ast::BlockNode> parse_block();

    // parse a namespace
    std::unique_ptr<ast::NamespaceNode> parse_namespace();

    // parse a code line in the top-level
    void parse_top_level_line(ast::ContainerNode& container);

    // parse a program from the top
    std::unique_ptr<ast::ProgramNode> parse();
  };

  // variables below contain the first sets for various structures
  namespace firstset {
    extern const lexer::TokenSet eol;
    extern const lexer::TokenSet top_level_line;
    extern const lexer::TokenSet line;
    extern const lexer::TokenSet literal;
    extern const lexer::TokenSet number;
    extern const lexer::TokenSet boolean;
    extern const lexer::TokenSet term;
    extern const lexer::TokenSet op;
    extern const lexer::TokenSet expression;
    extern const lexer::TokenSet type;
  }}
