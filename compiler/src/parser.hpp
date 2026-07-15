#pragma once

#include <set>
#include "lexer/lexer.hpp"
#include "messages/MessageWithSource.hpp"
#include "messages/list.hpp"
#include "ast/leaves/literal.hpp"
#include "ast/symbol_declaration.hpp"
#include "ast/function/function.hpp"
#include "ast/program.hpp"
#include "ast/function/return.hpp"
#include "ast/namespace.hpp"
#include "ast/operator.hpp"
#include "ast/function/operator_definition.hpp"
#include "ast/control-flow/if_statement.hpp"
#include "ast/control-flow/while_statement.hpp"
#include "ast/control-flow/loop_control.hpp"
#include "ast/control-flow/loop_statement.hpp"
#include "ast/leaves/array_literal.hpp"

/** @brief Recursive-descent parser turning a lexer's token stream into an AST. */
namespace lang::parser {
  /**
   * @brief Hand-written recursive-descent, precedence-climbing parser that consumes tokens from a `lexer::Lexer` and builds the AST.
   *
   * Buffers lookahead tokens and tracks the previously consumed token so grammar
   * rules can peek/backtrack cheaply. Exposes one `parse_*` method per grammar
   * construct, plus shared token-matching helpers; errors are reported into an
   * attached `message::List`.
   */
  class Parser {
    lexer::Lexer& lexer_; ///< Lexer tokens are read from.
    std::deque<lexer::Token> buffer_; ///< Buffered lookahead tokens, not yet consumed.
    lexer::Token prev_; ///< Most recently consumed token.
    message::List* messages_; ///< Message list errors are reported into, if attached.

    bool expect_block_end; ///< Whether the parser is at the last expression in a block.
    const lexer::Token* expr_last_; ///< Last token of the expression currently being parsed.

    /**
     * @brief Read up to `n` further tokens from the lexer into the lookahead buffer, stopping early at eof.
     * @param n Maximum number of tokens to read.
     */
    void read_tokens(unsigned int n);

    /**
     * @brief Add a message to the attached message list, if both are present.
     * @param m Message to add.
     */
    void add_message(std::unique_ptr<message::BasicMessage> m);

    /**
     * @brief Parse an expression using precedence climbing.
     * @param precedence Minimum operator precedence to accept before returning to the caller.
     * @return Parsed expression node.
     */
    std::unique_ptr<ast::Node> parse_expression_internal(int precedence);

    /** @brief Bundle of everything parsed after a function's name, handed to a caller-supplied factory to build the concrete function/operator node. */
    struct FunctionTailContent {
      lexer::Token token; ///< Start token of the function declaration.
      lexer::Token name; ///< Token holding the function's name.
      const type::FunctionNode& type; ///< Parsed function signature.
      std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> params; ///< Parsed parameter declarations.
      std::optional<std::unique_ptr<ast::BlockNode>> body; ///< Parsed body block, or empty for an unimplemented declaration.
    };

    /**
     * @brief Parse a function's parameter list, return type, and optional body, then hand the result to a factory to build the concrete node.
     * @param token_start Token at the start of the function declaration.
     * @param name Token holding the function's name.
     * @param create Factory callback that builds the concrete `ast::FunctionBaseNode` from the parsed content.
     * @return The node produced by `create`.
     */
    std::unique_ptr<ast::FunctionBaseNode> parse_function_tail(lexer::Token token_start, lexer::Token name, const std::function<std::unique_ptr<ast::FunctionBaseNode>(FunctionTailContent)>& create);

  public:
    /**
     * @brief Construct a parser reading from the given lexer.
     * @param lexer Lexer to pull tokens from.
     */
    explicit Parser(lexer::Lexer& lexer) : lexer_(lexer), prev_(lexer::Token::invalid(lexer.stream())) {}

    /** @brief Return the underlying lexer. */
    const lexer::Lexer& lexer() const { return lexer_; }

    /** @brief Return the attached message list, or nullptr if none is set. */
    message::List* messages() { return messages_; }

    /**
     * @brief Set the message list that parse errors/warnings are reported into.
     * @param messages Message list to attach.
     */
    void messages(message::List* messages) { messages_ = messages; }

    /** @brief Test whether any error-level message has been reported so far. */
    bool is_error() const;

    /** @brief Return the most recently consumed token. */
    const lexer::Token& previous() const { return prev_; }

    /**
     * @brief Peek at a token ahead of the current position without consuming it.
     * @param n Offset into the lookahead buffer; 0 is the next token.
     * @return The token at that offset (the eof token if past the end of input).
     */
    const lexer::Token& peek(unsigned int n = 0);

    /**
     * @brief Test whether the token at the given lookahead offset has one of the given types.
     * @param types Acceptable token types.
     * @param n Lookahead offset to check.
     * @return True if the token's type is in `types`.
     */
    bool expect(const lexer::TokenTypeSet& types, unsigned int n = 0);

    /**
     * @brief Test whether the token at the given lookahead offset has the given type.
     * @param type Acceptable token type.
     * @param n Lookahead offset to check.
     * @return True if the token's type matches.
     */
    bool expect(lexer::TokenType type, unsigned int n = 0);

    /**
     * @brief Test whether the token at the given lookahead offset matches one of the given tokens (type and image).
     * @param tokens Acceptable tokens.
     * @param n Lookahead offset to check.
     * @return True if the token matches one of `tokens`.
     */
    bool expect(const lexer::TokenSet& tokens, unsigned int n = 0);

    /**
     * @brief Test whether the token at the given lookahead offset matches the given token (type and image).
     * @param token Acceptable token.
     * @param n Lookahead offset to check.
     * @return True if the token matches.
     */
    bool expect(const lexer::BasicToken& token, unsigned int n = 0);

    /**
     * @brief Same as `expect(types)`, but reports a syntax-error message on failure.
     * @param types Acceptable token types.
     * @return True if the current token matches.
     */
    bool expect_or_error(const lexer::TokenTypeSet& types);

    /**
     * @brief Same as `expect(type)`, but reports a syntax-error message on failure.
     * @param type Acceptable token type.
     * @return True if the current token matches.
     */
    bool expect_or_error(lexer::TokenType type);

    /**
     * @brief Same as `expect(tokens)`, but reports a detailed syntax-error message on failure.
     * @param tokens Acceptable tokens.
     * @return True if the current token matches.
     */
    bool expect_or_error(const lexer::TokenSet& tokens);

    /**
     * @brief Same as `expect(token)`, but reports a detailed syntax-error message on failure.
     * @param token Acceptable token.
     * @return True if the current token matches.
     */
    bool expect_or_error(const lexer::BasicToken& token);

    /** @brief Consume and return the current token, which becomes the new "previous" token. */
    lexer::Token consume();

    /** @brief Parse a numeric or boolean literal. */
    std::unique_ptr<ast::LiteralNode> parse_literal();

    /** @brief Parse an array literal `[e1, e2, ...]`. */
    std::unique_ptr<ast::ArrayLiteralNode> parse_array_literal();

    /** @brief Parse a term: a number, symbol reference, bracketed expression, or similar atomic expression component. */
    std::unique_ptr<ast::Node> parse_term();

    /** @brief Parse a type expression. @return Pointer to the resolved type, or nullptr if invalid. */
    const type::Node* parse_type();

    /**
     * @brief Expect and consume a semicolon terminating an expression.
     * @param generate_messages Whether to report an error message if the semicolon is missing.
     * @return True if a semicolon was found and consumed.
     */
    bool check_semicolon_after_expression(bool generate_messages = true);

    /** @brief Controls whether `parse_expression` requires, allows, or forbids a trailing semicolon. */
    enum class ExprExpectSC {
      No, // no sc required or expected
      Maybe, // not requires but may be present (sets terminated_ property)
      Yes, // absolutely required
    };

    /**
     * @brief Parse an expression, optionally requiring/allowing a trailing semicolon.
     * @param expect_sc Whether a trailing semicolon is required, optional, or disallowed; updates the `expect_block_end` flag.
     * @param precedence Minimum operator precedence to accept.
     * @return Parsed expression node.
     */
    std::unique_ptr<ast::Node> parse_expression(ExprExpectSC expect_sc, int precedence = 0);

    /**
     * @brief Parse a `name: type` pair as a symbol declaration.
     * @param expect_type Whether a type annotation is required (a type is parsed if present regardless).
     * @return Parsed symbol declaration node.
     */
    std::unique_ptr<ast::SymbolDeclarationNode> parse_name_type_pair(bool expect_type);

    /**
     * @brief Parse a `let ...` or `const ...` statement, appending the resulting declaration(s) to a container.
     * @param container Container node to append the parsed declaration (and optional assignment expression) to.
     */
    void parse_var_decl(ast::ContainerNode& container);

    /** @brief Parse a parameter list `(arg: type, ...)`. @return Deque of parsed parameter declarations. */
    std::deque<std::unique_ptr<ast::SymbolDeclarationNode>> parse_param_list();

    /**
     * @brief Parse a function call's argument list `(...)` given an already-parsed callee.
     * @param subject Already-parsed callee expression.
     * @return Parsed function-call node.
     */
    std::unique_ptr<ast::FunctionCallOperatorNode> parse_function_call(std::unique_ptr<ast::Node> subject);

    /** @brief Parse a `func` statement (name, parameters, return type, and optional body). */
    std::unique_ptr<ast::FunctionBaseNode> parse_func();

    /** @brief Parse an `operator` definition statement. */
    std::unique_ptr<ast::FunctionBaseNode> parse_operator_definition();

    /** @brief Parse a `return` statement. */
    std::unique_ptr<ast::ReturnNode> parse_return();

    /** @brief Parse an `if ... else ...` statement. */
    std::unique_ptr<ast::IfStatementNode> parse_if_statement();

    /** @brief Parse a `while` statement. */
    std::unique_ptr<ast::WhileStatementNode> parse_while_statement();

    /** @brief Parse a `loop` statement. */
    std::unique_ptr<ast::LoopStatementNode> parse_loop_statement();

    /** @brief Parse a `break` or `continue` statement. */
    std::unique_ptr<ast::LoopControlNode> parse_loop_control_statement();

    /**
     * @brief Parse a `{ ... }` block if present, otherwise a single code line wrapped in a block.
     * @param in_top_level Whether this block occurs at the top level of the program.
     * @return Parsed block node.
     */
    std::unique_ptr<ast::BlockNode> parse_block_or_line(bool in_top_level);

    /**
     * @brief Parse a single code line and append it to the given block.
     * @param block Block to append the parsed line to.
     */
    void parse_line(ast::BlockNode& block);

    /**
     * @brief Parse a single code line, dispatching on whether it occurs at the top level.
     *
     * Checks the relevant first-set beforehand, so callers don't need to check first.
     * @param block Block to append the parsed line to.
     * @param top_level Whether this line occurs at the top level of the program.
     * @return True on success.
     */
    bool parse_line(ast::BlockNode& block, bool top_level);

    /** @brief Parse a `{ ... }` code block. */
    std::unique_ptr<ast::BlockNode> parse_block();

    /** @brief Parse a `namespace a.b.c { ... }` declaration. */
    std::unique_ptr<ast::NamespaceNode> parse_namespace();

    /** @brief Parse a `struct` declaration. */
    // TODO
    std::unique_ptr<ast::Node> parse_struct();

    /**
     * @brief Parse a single top-level code line and append it to the given container.
     * @param container Container to append the parsed line to.
     */
    void parse_top_level_line(ast::ContainerNode& container);

    /** @brief Parse an entire program from the start of the token stream. */
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
