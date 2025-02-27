#include "array_literal.hpp"
#include "ast/types/array.hpp"
#include "context.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"
#include "assembly/directive.hpp"
#include "value/future.hpp"

static unsigned int array_block_id = 0;

void lang::ast::ArrayLiteralNode::add(std::unique_ptr<Node> node) {
  token_end(node->token_end());
  elements_.push_back(std::move(node));
}

std::ostream& lang::ast::ArrayLiteralNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "[";
  for (int i = 0; i < elements_.size(); i++) {
    elements_[i]->print_code(os, indent_level);
    if (i < elements_.size() - 1) os << ", ";
  }
  return os << "]";
}

std::ostream& lang::ast::ArrayLiteralNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << SHELL_GREEN "length " SHELL_RESET SHELL_CYAN << elements_.size() << SHELL_RESET;

  for (auto& element : elements_) {
    os << std::endl;
    element->print_tree(os, indent_level + 1);
  }
  return os;
}

bool lang::ast::ArrayLiteralNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  for (auto& el : elements_) {
    if (!el->collate_registry(messages, registry)) return false;
  }
  return true;
}

bool lang::ast::ArrayLiteralNode::process(lang::Context& ctx) {
  // this will hold the inner type of the array
  optional_ref<const type::Node> inner_type;

  if (type_hint() && type_hint()->get().get_array()) {
   inner_type = type_hint()->get().get_array()->unwrap();
  }

  // process & match types of each element
  for (auto& element : elements_) {
    element->type_hint(inner_type);
    if (!element->process(ctx) || !element->resolve(ctx)) return false;

    // if type is already given, check if matches
    // TODO allow sub and super types
    auto& type = element->value().type();
    if (inner_type) {
      if (!type::graph.is_subtype(type.id(), inner_type->get().id())) {
        ctx.messages.add(util::error_type_mismatch(*element, type, inner_type->get(), false));
        return false;
      }
    } else {
      inner_type = type;
    }
  }

  // if we still don't have a type - error
  if (!inner_type) {
    auto msg = generate_message(message::Error);
    msg->get() << "unable to deduce array type - must be non-empty or provide explicit type";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // accumulate element's values
  value::ContiguousLiteral::Elements element_values;
  for (auto& element : elements_) {
    element_values.push_back(element->value());
  }

  // set value_
  auto& array_type = type::ArrayNode::get(inner_type->get(), elements_.size());
  value_ = value::contiguous_literal(array_type, element_values, ctx.symbols.in_global_scope());
  return true;
}

bool lang::ast::ArrayLiteralNode::generate_code(Context& ctx) {
  // generate code for each of our values
  for (auto& element : elements_) {
    if (!element->generate_code(ctx)) return false;
  }
  return true;
}
