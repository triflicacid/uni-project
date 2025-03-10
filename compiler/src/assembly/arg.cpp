#include "arg.hpp"
#include "basic_block.hpp"
#include <cassert>

std::ostream& lang::assembly::Arg::print(std::ostream& os) const {
  switch (type_) {
    case constants::inst::imm:
      return os << "0x" << std::hex << value_ << std::dec;
    case constants::inst::reg:
      return os << "$" << constants::registers::to_string($reg(value_));
    case constants::inst::mem:
      return os << "(0x" << std::dec << value_ << std::dec << ")";
    case constants::inst::reg_indirect:
      if (uint32_t offset = value_ >> 8) {
        if (offset & 0x800000) offset |= 0xff000000; // recover the sign bit
        os << *(int32_t*)&offset;
      }
      return os << "($" << constants::registers::to_string($reg(value_ & 0xff)) << ")";
    default:
      return os << "arg?";
  }
}

std::unique_ptr<lang::assembly::Arg> lang::assembly::Arg::imm(uint32_t x) {
  return std::make_unique<Arg>(constants::inst::imm, x);
}

std::unique_ptr<lang::assembly::Arg> lang::assembly::Arg::reg(uint8_t reg) {
  return std::make_unique<Arg>(constants::inst::reg, reg);
}

std::unique_ptr<lang::assembly::Arg> lang::assembly::Arg::mem(uint32_t addr) {
  return std::make_unique<Arg>(constants::inst::mem, addr);
}

std::unique_ptr<lang::assembly::Arg> lang::assembly::Arg::reg_indirect(uint8_t reg, int32_t offset) {
  uint32_t shifted = (*(uint32_t*)&offset) << 8;
  return std::make_unique<Arg>(constants::inst::reg_indirect, shifted | reg);
}

std::unique_ptr<lang::assembly::LabelArg> lang::assembly::Arg::label(const std::string& label, int offset, bool is_addr) {
  return std::make_unique<LabelArg>(label, offset, is_addr);
}

std::unique_ptr<lang::assembly::BlockReferenceArg> lang::assembly::Arg::label(const lang::assembly::BasicBlock& block, int offset, bool is_addr) {
  return std::make_unique<BlockReferenceArg>(block, offset, is_addr);
}

std::unique_ptr<lang::assembly::BaseArg> lang::assembly::Arg::copy() const {
  return std::make_unique<Arg>(type_, value_);
}

std::ostream& lang::assembly::LabelArg::print(std::ostream& os) const {
  if (addr_) {
    if (offset_ != 0) os << offset_;
    os << "(" << label_ << ")";
  } else {
    os << label_;
    if (offset_ != 0) os << " + " << offset_;
  }
  return os;
}

std::unique_ptr<lang::assembly::BaseArg> lang::assembly::LabelArg::copy() const {
  return std::make_unique<LabelArg>(label_, offset_, addr_);
}

std::ostream& lang::assembly::BlockReferenceArg::print(std::ostream& os) const {
  if (addr_) {
    if (offset_ != 0) os << offset_;
    os << "(" << block_.label() << ")";
  } else {
    os << block_.label();
    if (offset_ != 0) os << " + " << offset_;
  }
  return os;
}

std::unique_ptr<lang::assembly::BaseArg> lang::assembly::BlockReferenceArg::copy() const {
  return std::make_unique<BlockReferenceArg>(block_, offset_, addr_);
}
