#pragma once

#include "node.hpp"
#include "ast/types/node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast {
  class SymbolDeclarationNode : public Node {
  public:
    enum Category {
      Variable,
      Argument,
      Constant
    };

  private:
    lexer::Token name_;
    std::optional<std::reference_wrapper<const type::Node>> type_; // optional if type deduction required
    Category category_ = Variable;
    symbol::SymbolId id_; // ID of created symbol
    std::optional<std::unique_ptr<Node>> assignment_; // optional assignment

  public:
    SymbolDeclarationNode(lexer::Token token, lexer::Token name, std::optional<std::reference_wrapper<const type::Node>> type)
      : Node(std::move(token)), name_(std::move(name)), type_(std::move(type)) {}

    SymbolDeclarationNode(lexer::Token token, lexer::Token name, std::optional<std::reference_wrapper<const type::Node>> type, std::optional<std::unique_ptr<Node>> assignment)
      : Node(std::move(token)), name_(std::move(name)), type_(std::move(type)), assignment_(std::move(assignment)) {}

    std::string node_name() const override;

    const lexer::Token& name() const { return name_; }

    // set assignment content
    void assign_to(std::unique_ptr<Node> expr) { assignment_ = std::move(expr); }

    Category category() const { return category_; }

    void set_category(Category c) { category_ = c; }

    const type::Node& type() const;

    // get ID of created function
    symbol::SymbolId id() const { return id_; }

    bool writes_to_ret() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override;

    bool generate_code(lang::Context &ctx) override;
  };
}
