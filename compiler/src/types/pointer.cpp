#include "pointer.hpp"

std::ostream& lang::type::PointerNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "*";
  unwrap().print_code(os, indent_level);
  return os;
}

const lang::type::PointerNode& lang::type::PointerNode::get(const Node& inner) {
  return WrapperNode::get<PointerNode>("pointer", inner, [&] {
    return std::make_unique<PointerNode>(inner);
  });
}
