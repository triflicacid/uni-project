#include "literal.hpp"
#include "uint64.hpp"
#include "ast/types/node.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"

std::string lang::memory::Literal::to_string() const {
  if (auto int_ = type_.get_int()) {
    return int_->is_signed()
           ? std::to_string(uint64::to_int64(data_))
           : std::to_string(uint64::to_uint64(data_));
  } else {
    auto float_ = type_.get_float();
    return float_->is_double()
           ? std::to_string(uint64::to_double(data_))
           : std::to_string(uint64::to_float(data_));
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
