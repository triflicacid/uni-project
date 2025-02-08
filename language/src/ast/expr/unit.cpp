#include "unit.hpp"

std::ostream& lang::ast::UnitNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "()";
}

bool lang::ast::UnitNode::process(lang::Context& ctx) {
  return true;
}

const lang::value::Value& lang::ast::UnitNode::value() const {
  return value::unit_value;
}
