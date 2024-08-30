#include <processor/src/constants.h>

#include "instruction.hpp"
#include "transform.hpp"
#include "util.hpp"

namespace assembler::instruction {
  std::unordered_map<std::string, uint8_t> conditional_postfix_map = {
    { "z", CMP_Z },
    { "nz", CMP_NZ },
    { "neq", CMP_NE },
    { "ne", CMP_NE },
    { "eq", CMP_EQ },
    { "lte", CMP_LE },
    { "lt", CMP_LT },
    { "le", CMP_LE },
    { "gte", CMP_GE },
    { "gt", CMP_GT },
    { "ge", CMP_GE },
    };

  std::map<std::string, uint8_t> datatype_postfix_map = {
    { "hu", DATATYPE_U32 },
    { "u", DATATYPE_U64 },
    { "hi", DATATYPE_S32 },
    { "i", DATATYPE_S64 },
    { "f", DATATYPE_F },
    { "d", DATATYPE_D }
  };

  Signature *find_signature(const std::string &mnemonic, std::string &options) {
    for (auto &signature : signature_list) {
      if (starts_with(mnemonic, signature.mnemonic)) {
        options = mnemonic.substr(signature.mnemonic.size());
        return &signature;
      }
    }

    return nullptr;
  }

  Signature *find_signature(const std::string &mnemonic) {
    for (auto &signature : signature_list) {
      if (starts_with(mnemonic, signature.mnemonic)) {
        return &signature;
      }
    }

    return nullptr;
  }

  const std::deque reg_val = { ArgumentType::Register, ArgumentType::Value };
  const std::deque reg_reg_val = { ArgumentType::Register, ArgumentType::Register, ArgumentType::Value };
  std::vector<Signature> signature_list = {
    { "add", OP_ADD, true, true, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "and", OP_AND, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "b", 0x00, true, false, { { ArgumentType::Value } }, false, transform::branch },
    { "call", OP_CALL, true, false, { { ArgumentType::Address } }, false, nullptr },
    { "div", OP_DIV, true, true, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "exit", 0x00, true, false, { { }, { ArgumentType::Value } }, false, transform::exit },
    { "jmp", 0x00, false, false, { { ArgumentType::Value } }, false, transform::jump },
    { "loadu", OP_LOAD_UPPER, true, false, { reg_val }, false, nullptr },
    { "loadw", 0x00, true, false, { reg_val }, true, transform::loadw },
    { "load", OP_LOAD, true, false, { reg_val }, false, nullptr },
    { "mod", OP_MOD, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "mul", OP_MUL, true, true, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "nop", OP_NOP, false, false, { { } }, false, nullptr },
    { "not", OP_NOT, true, false, { { ArgumentType::Register }, { ArgumentType::Register, ArgumentType::Register } }, false, transform::duplicate_reg },
    { "or", OP_OR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "pushw", 0, true, false, { { ArgumentType::Value } }, true, transform::pushw },
    { "push", OP_PUSH, true, false, { { ArgumentType::Value } }, false, nullptr },
    { "ret", OP_RET, true, false, { { } }, false, nullptr },
    { "shl", OP_SHL, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "shr", OP_SHR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "store", OP_STORE, true, false, { { ArgumentType::Register, ArgumentType::Address } }, false, nullptr },
    { "sub", OP_SUB, true, true, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "syscall", OP_SYSCALL, true, false, { { ArgumentType::Value } }, false, nullptr },
    { "xor", OP_XOR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "zero", 0x00, true, false, { { ArgumentType::Register } }, false, transform::zero },
  };

}
