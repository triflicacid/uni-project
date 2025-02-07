#include "pointer.hpp"

std::ostream& lang::ast::type::PointerNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "*";
  unwrap().print_code(os, indent_level);
  return os;
}

const lang::ast::type::PointerNode& lang::ast::type::PointerNode::create(const lang::ast::type::Node& inner) {
  return WrapperNode::create<PointerNode>("pointer", inner, [&] {
    return std::make_unique<PointerNode>(inner);
  });
}
