#include "unit.hpp"

std::ostream& lang::type::UnitNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "()";
}

constants::inst::datatype::dt lang::type::UnitNode::get_asm_datatype() const {
  throw std::runtime_error("unit::get_asm_type should never be called");
}

lang::type::UnitNode lang::type::unit;
