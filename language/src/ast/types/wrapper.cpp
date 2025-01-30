#include "wrapper.hpp"

std::ostream& lang::ast::type::WrapperNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << name_ << "<";
  inner_.print_code(os);
  return os << ">";
}

std::string lang::ast::type::WrapperNode::to_label() const {
  return name_ + inner_.to_label();
}
