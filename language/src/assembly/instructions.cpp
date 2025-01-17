#include "instructions.hpp"

using namespace lang::assembly;

// create reg_value instruction
static std::unique_ptr<GenericInstruction> create_reg_value(const std::string& mnemonic, uint8_t reg, std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>(mnemonic);
  inst->add_arg(Arg::reg(reg));
  inst->add_arg(std::move(value));
  return inst;
}

// create reg_reg_value instruction
static std::unique_ptr<GenericInstruction> create_reg_reg_value(const std::string& mnemonic, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  auto inst = std::make_unique<GenericInstruction>(mnemonic);
  inst->add_arg(Arg::reg(reg1));
  inst->add_arg(Arg::reg(reg2));
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

std::unique_ptr<GenericInstruction> lang::assembly::create_mod(datatype datatype, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("mod", reg1, reg2, std::move(value));
}

std::unique_ptr<GenericInstruction> lang::assembly::create_mul(datatype datatype, uint8_t reg1, uint8_t reg2, std::unique_ptr<BaseArg> value) {
  return create_reg_reg_value("mul", reg1, reg2, std::move(value));
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

std::unique_ptr<GenericInstruction> lang::assembly::create_store(uint8_t reg, uint32_t address) {
  auto inst = std::make_unique<GenericInstruction>("store");
  inst->add_arg(Arg::reg(reg));
  inst->add_arg(Arg::mem(address));
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

std::unique_ptr<Instruction> lang::assembly::create_return_from_interrupt() {
  return std::make_unique<Instruction>("rti");
}

std::unique_ptr<LoadImmediateInstruction> lang::assembly::create_load_long(uint8_t reg, uint64_t imm) {
  return std::make_unique<LoadImmediateInstruction>(reg, imm);
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
