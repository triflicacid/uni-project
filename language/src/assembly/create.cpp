#include "create.hpp"

using namespace lang::assembly;

std::unique_ptr<GenericInstruction> lang::assembly::instruction(const std::string& mnemonic) {
  return std::make_unique<GenericInstruction>(mnemonic);
}

// create reg_value instruction
static std::unique_ptr<GenericInstruction> create_reg_value(const std::string& mnemonic, uint8_t reg, std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>(mnemonic);
  inst->add_arg(Arg::reg(reg));
  inst->add_arg(std::move(value));
  return inst;
}

// create reg_reg_value instruction
// emit reg_value if reg1 == reg2
static std::unique_ptr<GenericInstruction> create_reg_reg_value(const std::string& mnemonic, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>(mnemonic);
  inst->add_arg(Arg::reg(reg1));
  if (reg1 != reg2) inst->add_arg(Arg::reg(reg2));
  inst->add_arg(std::move(value));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_add(datatype datatype, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  auto inst = create_reg_reg_value("add", reg1, reg2, std::move(value));
  inst->set_datatype(datatype);
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_and(uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("and", reg1, reg2, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_div(datatype datatype, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  auto inst = create_reg_reg_value("div", reg1, reg2, std::move(value));
  inst->set_datatype(datatype);
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_jump_and_link(std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>("jal");
  inst->add_arg(std::move(value));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_load(uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_value("load", reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_load_upper(uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_value("loadu", reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_mod(uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("mod", reg1, reg2, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_mul(datatype datatype, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  auto inst = create_reg_reg_value("mul", reg1, reg2, std::move(value));
  inst->set_datatype(datatype);
  return inst;
}

std::unique_ptr<Instruction> lang::assembly::create_nop() {
  return std::make_unique<Instruction>("nop");
}

std::unique_ptr<GenericInstruction> lang::assembly::create_not(uint8_t reg_dst, uint8_t reg) {
  auto inst = std::make_unique<GenericInstruction>("not");
  inst->add_arg(Arg::reg(reg_dst));
  inst->add_arg(Arg::reg(reg));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_or(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("or", reg_dst, reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_signed_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm) {
  auto inst = std::make_unique<GenericInstruction>("sext");
  inst->add_arg(Arg::reg(reg_dst));
  inst->add_arg(std::move(value));
  inst->add_arg(Arg::imm(imm));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_shift_left(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("shl", reg_dst, reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_shift_right(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("shr", reg_dst, reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_store(uint8_t reg, std::unique_ptr<BaseArg> address) {
  auto inst = std::make_unique<GenericInstruction>("store");
  inst->add_arg(Arg::reg(reg));
  inst->add_arg(std::move(address));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_sub(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("sub", reg_dst, reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_system_call(std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>("syscall");
  inst->add_arg(std::move(value));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_xor(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("xor", reg_dst, reg, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_zero_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm) {
  auto inst = std::make_unique<GenericInstruction>("zext");
  inst->add_arg(Arg::reg(reg_dst));
  inst->add_arg(std::move(value));
  inst->add_arg(Arg::imm(imm));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_branch(std::unique_ptr<BaseArg> to) {
  auto inst = std::make_unique<GenericInstruction>("jmp");
  inst->add_arg(std::move(to));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_branch(condition guard, std::unique_ptr<BaseArg> to) {
  auto inst = std::make_unique<GenericInstruction>("b");
  inst->set_conditional(guard);
  inst->add_arg(std::move(to));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_exit(std::unique_ptr<BaseArg> exit_code) {
  auto inst = std::make_unique<GenericInstruction>("b");
  inst->add_arg(std::move(exit_code));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_extend(bool is_signed, uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm) {
  auto &func = is_signed ? create_signed_extend : create_zero_extend;
  return func(reg_dst, std::move(value), imm);
}


std::unique_ptr<GenericInstruction> lang::assembly::create_push(uint8_t bytes) {
  auto inst = std::make_unique<GenericInstruction>("sub");
  inst->add_arg(Arg::reg(constants::registers::sp));
  inst->add_arg(Arg::imm(bytes));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_pop(uint8_t bytes) {
  auto inst = std::make_unique<GenericInstruction>("add");
  inst->add_arg(Arg::reg(constants::registers::sp));
  inst->add_arg(Arg::imm(bytes));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_zero(uint8_t reg) {
  auto inst = std::make_unique<GenericInstruction>("zero");
  inst->add_arg(Arg::reg(reg));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_return() {
  return std::make_unique<GenericInstruction>("ret");
}

std::unique_ptr<GenericInstruction> lang::assembly::create_return(std::unique_ptr<BaseArg> value) {
  auto inst = create_return();
  inst->add_arg(std::move(value));
  return inst;
}

std::unique_ptr<GenericInstruction> lang::assembly::create_return_from_interrupt() {
  return std::make_unique<GenericInstruction>("rti");
}

std::unique_ptr<LoadImmediateInstruction> lang::assembly::create_load_long(uint8_t reg, uint64_t imm) {
  return std::make_unique<LoadImmediateInstruction>(reg, imm);
}

std::unique_ptr<GenericInstruction> lang::assembly::create_comparison(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value) {
  auto inst = create_reg_reg_value("cmp", reg_dst, reg, std::move(value));
  inst->set_datatype(datatype);
  return inst;
}

std::unique_ptr<ConversionInstruction>
lang::assembly::create_conversion(datatype from_type, uint8_t from_reg, datatype to_type, uint8_t to_reg) {
  return std::make_unique<ConversionInstruction>(from_type, from_reg, to_type, to_reg);
}

std::unique_ptr<GenericInstruction> lang::assembly::create_interrupt(std::unique_ptr<BaseArg> mask) {
  auto inst = std::make_unique<GenericInstruction>("int");
  inst->add_arg(std::move(mask));
  return inst;
}

std::unique_ptr<Instruction> lang::assembly::create_exit() {
  return std::make_unique<Instruction>("exit");
}

void lang::assembly::create_load(uint8_t reg, std::unique_ptr<BaseArg> value, uint8_t bytes, BasicBlock& block, bool is_signed) {
  // start with a normal load
  block.add(create_load(reg, std::move(value)));

  // clear remainder of register
  if (bytes < 8) {
    block.add(create_extend(is_signed, reg, Arg::reg(reg), (8 - bytes) * 8));
  }
}

void lang::assembly::create_store(uint8_t reg, std::unique_ptr<BaseArg> address, uint8_t bytes, BasicBlock& block) {
  // TODO implemented sized store
  block.add(create_store(reg, std::move(address)));
  return;

  // if bytes exceeds a word, we do nothing
  if (bytes > 7) {
    block.add(create_store(reg, std::move(address)));
    return;
  }

  // otherwise, pick a register (must be register) to use as a temporary
}
