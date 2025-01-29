#include "instruction.hpp"

using namespace lang::assembly;

std::ostream& GenericInstruction::_print(std::ostream& os) const {
  // print mnemonic + optional decorators
  Instruction::_print(os);
  if (cond_.has_value()) os << constants::cmp::to_string(cond_.value());
  if (datatype_.has_value()) os << "." << constants::inst::datatype::to_string(datatype_.value());
  os << " ";

  // print comma-separated arguments
  int i = 0;
  for (auto& arg : args_) {
    arg->print(os);
    if (++i < args_.size()) os << ", ";
  }

  return os;
}

GenericInstruction& GenericInstruction::set_conditional(constants::cmp::flag cmp) {
  cond_ = cmp;
  return *this;
}

GenericInstruction& GenericInstruction::set_datatype(constants::inst::datatype::dt dt) {
  datatype_ = dt;
  return *this;
}

GenericInstruction& GenericInstruction::add_arg(std::unique_ptr<BaseArg> arg) {
  args_.push_back(std::move(arg));
  return *this;
}

std::ostream& ConversionInstruction::_print(std::ostream& os) const {
  Instruction::_print(os);
  os << constants::inst::datatype::to_string(from_type_) << "2"
     << constants::inst::datatype::to_string(to_type_) << " "
     << "$" << constants::registers::to_string($reg(from_reg_)) << ", "
     << "$" << constants::registers::to_string($reg(to_reg_));
  return os;
}

std::ostream& LoadImmediateInstruction::_print(std::ostream& os) const {
  Instruction::_print(os) << " ";
  os << "$" << constants::registers::to_string($reg(reg_)) << ", "
     << "0x" << std::hex << imm_ << std::dec;
  return os;
}
