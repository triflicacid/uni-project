#include "literal.hpp"
#include "uint64.hpp"
#include "ast/types/node.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "config.hpp"

std::string lang::memory::Literal::to_string() const {
  if (auto int_ = type_.get_int()) {
    return int_->is_signed()
           ? std::to_string(uint64::to_int64(data_))
           : std::to_string(uint64::to_uint64(data_));
  } else if (auto float_ = type_.get_float()) {
    return float_->is_double()
           ? std::to_string(uint64::to_double(data_))
           : std::to_string(uint64::to_float(data_));
  } else { // must be a Boolean
    return data_ == 0
           ? lang::conf::bools::false_string
           : lang::conf::bools::true_string;
  }
}

// store Literals, indexed by <typeID, data>
static std::map<std::pair<lang::ast::type::TypeId, uint64_t>, std::unique_ptr<lang::memory::Literal>> literals;

const lang::memory::Literal& lang::memory::Literal::get(const lang::ast::type::Node& type, uint64_t data) {
  const auto key = std::make_pair(type.id(), data);
  if (auto it = literals.find(key); it != literals.end()) {
    return *it->second;
  }

  std::unique_ptr<Literal> literal(new Literal(type, data));
  literals.insert({key, std::move(literal)});
  return *literals.at(key);
}

const lang::memory::Literal& lang::memory::Literal::zero(const lang::ast::type::Node& type) {
  static const float zero_f32 = 0;
  static const double zero_f64 = 0;

  uint64_t data;
  switch (type.get_asm_datatype()) {
    case constants::inst::datatype::flt:
      data = *(uint32_t*)&zero_f32;
      break;
    case constants::inst::datatype::dbl:
      data = *(uint64_t*)&zero_f64;
      break;
    default:
      data = 0x0;
  }

  return get(type, data);
}
