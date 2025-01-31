#include "node.hpp"
#include "int.hpp"
#include "float.hpp"

static lang::ast::type::TypeId current_id = 0;

lang::ast::type::Node::Node(): id_(current_id++) {}

const lang::ast::type::Node& lang::ast::type::from_asm_type(constants::inst::datatype::dt type) {
  switch (type) {
    case constants::inst::datatype::u32:
      return uint32;
    case constants::inst::datatype::u64:
      return uint64;
    case constants::inst::datatype::s32:
      return int32;
    case constants::inst::datatype::s64:
      return int64;
    case constants::inst::datatype::flt:
      return float32;
    case constants::inst::datatype::dbl:
      return float64;
  }
}

const std::deque<std::reference_wrapper<const lang::ast::type::Node>> lang::ast::type::numerical{
  uint8, int8,
  uint16, int16,
  uint32, int32,
  uint64, int64,
  float32, float64,
};
