#include "literal.hpp"
#include "uint64.hpp"
#include "ast/types/node.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "config.hpp"
#include "ast/types/bool.hpp"
#include "ast/types/array.hpp"

std::string lang::memory::Literal::to_string() const {
  if (auto int_ = type_.get_int()) {
    return int_->is_signed()
           ? std::to_string(uint64::to_int64(data_))
           : std::to_string(uint64::to_uint64(data_));
  } else if (auto float_ = type_.get_float()) {
    return float_->is_double()
           ? std::to_string(uint64::to_double(data_))
           : std::to_string(uint64::to_float(data_));
  } else if (type_ == ast::type::boolean) { // must be a Boolean
    return data_ == 0
           ? lang::conf::bools::false_string
           : lang::conf::bools::true_string;
  } else {
    return std::to_string(data_);
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

const lang::memory::Literal& lang::memory::Literal::get_boolean(bool b) {
  return get(ast::type::boolean, b ? 1 : 0);
}

bool lang::memory::Literal::operator==(const lang::memory::Literal& other) const {
  if (type_.get_array() || other.type_.get_array()) return false; // arrays are never equal
  return type_ == other.type_ && data_ == other.data_;
}

const lang::memory::Literal& lang::memory::Literal::change_type(const ast::type::Node& target) const {
  // get internal datatypes
  auto from = type_.get_asm_datatype();
  auto to = target.get_asm_datatype();

  // if equal, return copy now
  if (from == to) return *this;

  // otherwise, convert
  uint64_t new_data = 0;
  switch (from) {
    case constants::inst::datatype::u32:
      switch (to) {
        case constants::inst::datatype::s32:
          new_data = uint64::from((int32_t) uint64::to_uint32(data_));
          break;
        case constants::inst::datatype::s64:
          new_data = uint64::from((int64_t) uint64::to_uint32(data_));
          break;
        case constants::inst::datatype::flt:
          new_data = uint64::from((float) uint64::to_uint32(data_));
          break;
        case constants::inst::datatype::dbl:
          new_data = uint64::from((double) uint64::to_uint32(data_));
          break;
        default: ;
      }
      break;
    case constants::inst::datatype::u64:
      switch (to) {
        case constants::inst::datatype::u32:
          new_data = uint64::from((uint32_t) data_);
          break;
        case constants::inst::datatype::s32:
          new_data = uint64::from((int32_t) data_);
          break;
        case constants::inst::datatype::s64:
          new_data = uint64::from((int64_t) data_);
          break;
        case constants::inst::datatype::flt:
          new_data = uint64::from((float) data_);
          break;
        case constants::inst::datatype::dbl:
          new_data = uint64::from((double) data_);
          break;
        default: ;
      }
      break;
    case constants::inst::datatype::s32:
      switch (to) {
        case constants::inst::datatype::u32:
          new_data = uint64::from((uint32_t) uint64::to_int32(data_));
          break;
        case constants::inst::datatype::u64:
          new_data = uint64::from((uint64_t) uint64::to_int32(data_));
          break;
        case constants::inst::datatype::s64:
          new_data = uint64::from((int64_t) uint64::to_int32(data_));
          break;
        case constants::inst::datatype::flt:
          new_data = uint64::from((float) uint64::to_int32(data_));
          break;
        case constants::inst::datatype::dbl:
          new_data = uint64::from((double) uint64::to_int32(data_));
          break;
        default: ;
      }
      break;
    case constants::inst::datatype::s64:
      switch (to) {
        case constants::inst::datatype::u32:
          new_data = uint64::from((uint32_t) uint64::to_int64(data_));
          break;
        case constants::inst::datatype::u64:
          new_data = uint64::from((uint64_t) uint64::to_int64(data_));
          break;
        case constants::inst::datatype::s32:
          new_data = uint64::from((int32_t) uint64::to_int64(data_));
          break;
        case constants::inst::datatype::flt:
          new_data = uint64::from((float) uint64::to_int64(data_));
          break;
        case constants::inst::datatype::dbl:
          new_data = uint64::from((double) uint64::to_int64(data_));
          break;
        default: ;
      }
      break;
    case constants::inst::datatype::flt:
      switch (to) {
        case constants::inst::datatype::u32:
          new_data = uint64::from((uint32_t) uint64::to_float(data_));
          break;
        case constants::inst::datatype::u64:
          new_data = uint64::from((uint64_t) uint64::to_float(data_));
          break;
        case constants::inst::datatype::s32:
          new_data = uint64::from((int32_t) uint64::to_float(data_));
          break;
        case constants::inst::datatype::s64:
          new_data = uint64::from((int64_t) uint64::to_float(data_));
          break;
        case constants::inst::datatype::dbl:
          new_data = uint64::from((double) uint64::to_float(data_));
          break;
        default: ;
      }
      break;
    case constants::inst::datatype::dbl:
      switch (to) {
        case constants::inst::datatype::u32:
          new_data = uint64::from((uint32_t) uint64::to_double(data_));
          break;
        case constants::inst::datatype::u64:
          new_data = uint64::from((uint64_t) uint64::to_double(data_));
          break;
        case constants::inst::datatype::s32:
          new_data = uint64::from((int32_t) uint64::to_double(data_));
          break;
        case constants::inst::datatype::s64:
          new_data = uint64::from((int64_t) uint64::to_double(data_));
          break;
        case constants::inst::datatype::flt:
          new_data = uint64::from((float) uint64::to_double(data_));
          break;
        default: ;
      }
      break;
  }

  return get(target, new_data);
}
