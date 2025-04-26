#include "float.hpp"

std::string lang::type::FloatNode::node_name() const {
  return double_ ? "f64" : "f32";
}

std::ostream &lang::type::FloatNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << (double_ ? "f64" : "f32");
}

constants::inst::datatype::dt lang::type::FloatNode::get_asm_datatype() const {
  return double_
    ? constants::inst::datatype::dbl
    : constants::inst::datatype::flt;
}

std::string lang::type::FloatNode::to_label() const {
  return node_name();
}

lang::type::FloatNode
  lang::type::float32(false),
  lang::type::float64(true);
