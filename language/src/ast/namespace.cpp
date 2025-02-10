#include "namespace.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "config.hpp"
#include "ast/types/namespace.hpp"

std::string lang::ast::NamespaceNode::name() const {
  std::stringstream stream;
  for (int i = 0; i < names_.size(); i++) {
    stream << names_[i].image;
    if (i + 1 < names_.size()) stream << ".";
  }
  return stream.str();
}


std::ostream& lang::ast::NamespaceNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "namespace " << name() << " {";
  for (auto& line : lines_) {
    os << std::endl;
    indent(os, indent_level + 1);
    line->print_code(os, indent_level + 1);
  }
  os << std::endl;
  indent(os, indent_level);
  return os << "}";
}

std::ostream& lang::ast::NamespaceNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << SHELL_GREEN << name() << SHELL_RESET;
  for (auto& line : lines_) {
    os << std::endl;
    line->print_tree(os, indent_level + 1);
  }
  return os;
}

bool lang::ast::NamespaceNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  assert(!names_.empty());

  // iterate through our path
  // all names either don't exist, or if they do, must be a namespace already
  std::string namespace_name; // cumulative name
  for (const lexer::Token& name : names_) {
    // add to cumulative name
    if (!namespace_name.empty()) namespace_name += ".";
    namespace_name += name.image;

    // get any other symbols which exist with this name
    auto& others = registry.get(namespace_name);

    // if no other symbol exists, create it as a namespace
    if (others.empty()) {
      auto symbol = symbol::create_namespace(name);

      // if a namespace was created before us, add it as a parent
      if (id_.has_value()) {
        symbol->set_parent(registry.get(*id_));
      }

      id_ = symbol->id();
      registry.insert(std::move(symbol));
      continue;
    }

    // otherwise, check that they are namespaces and error if not
    for (symbol::SymbolId other_id : others) {
      const symbol::Symbol& other = registry.get(other_id);
      if (other.type() == type::name_space) {
        // cool! set id_
        id_ = other.id();
      } else {
        auto msg = name.generate_message(message::Error);
        msg->get() << "symbol " << namespace_name << " used as a namespace, but is defined elsewhere";
        messages.add(std::move(msg));

        msg = other.token().generate_message(message::Note);
        msg->get() << other.full_name() << " previously defined here";
        messages.add(std::move(msg));
        return false;
      }
    }
  }

  // collate body in a separate registry
  registry_ = std::make_unique<symbol::Registry>();
  for (auto& line : lines_) {
    if (!line->collate_registry(messages, *registry_)) return false;
  }

  // add us as a parent to all collated symbols
  auto& ns = registry.get(*id_);
  for (auto& [symbolId, symbol] : *registry_) {
    symbol->set_parent(ns);
  }

  return true;
}

bool lang::ast::NamespaceNode::process(lang::Context& ctx) {
  assert(registry_ != nullptr);
  assert(id_.has_value());

  // record nested structure of namespace
  ctx.symbols.push_path(*id_);

  // insert into symbol table
  ctx.symbols.insert(*registry_);

  // process children
  for (auto& line : lines_) {
    if (!line->process(ctx)) return false;
  }

  ctx.symbols.pop_path();
  return true;
}

void lang::ast::NamespaceNode::add(std::unique_ptr<Node> ast_node) {
  lines_.push_back(std::move(ast_node));
}

void lang::ast::NamespaceNode::add(std::deque<std::unique_ptr<Node>> ast_nodes) {
  while (!ast_nodes.empty()) {
    lines_.push_back(std::move(ast_nodes.front()));
    ast_nodes.pop_front();
  }
}
