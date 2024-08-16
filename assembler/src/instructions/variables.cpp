#include <processor/src/constants.h>

#include "instruction.hpp"
#include "transform.hpp"
#include "util.hpp"

namespace assembler::instruction {
  std::map<std::string, uint8_t> conditional_postfix_map = {
    { "z", CMP_Z },
    { "nz", CMP_NZ },
    { "eq", CMP_EQ },
    { "ne", CMP_NE },
    { "lt", CMP_LT },
    { "le", CMP_LE },
    { "gt", CMP_GT },
    { "ge", CMP_GE },
    };

  std::map<std::string, uint8_t> datatype_postfix_map = {
    { "hu", DATATYPE_U32 },
    { "u", DATATYPE_U64 },
    { "hs", DATATYPE_S32 },
    { "s", DATATYPE_S64 },
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

  const std::deque reg_val = { ArgumentType::Register, ArgumentType::Value };
  const std::deque reg_reg_val = { ArgumentType::Register, ArgumentType::Register, ArgumentType::Value };

  std::vector<Signature> signature_list = {
    { "and", OP_AND, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "b", 0x00, true, false, { { ArgumentType::Value } }, false, transform::branch },
    { "exit", 0x00, true, false, { { }, { ArgumentType::Value } }, false, transform::exit },
    { "jmp", 0x00, false, false, { { ArgumentType::Value } }, false, transform::jump },
    { "loadu", OP_LOAD_UPPER, true, false, { reg_val }, false, nullptr },
    { "loadw", 0x00, true, false, { reg_val }, true, transform::loadw },
    { "load", OP_LOAD, true, false, { reg_val }, false, nullptr },
    { "nop", OP_NOP, false, false, { { } }, false, nullptr },
    { "not", OP_NOT, true, false, { { ArgumentType::Register }, { ArgumentType::Register, ArgumentType::Register } }, false, transform::duplicate_reg },
    { "or", OP_OR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "shl", OP_SHL, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "shr", OP_SHR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "store", OP_STORE, true, false, { { ArgumentType::Register, ArgumentType::Address } }, false, nullptr },
    { "syscall", OP_SYSCALL, true, false, { { ArgumentType::Value } }, false, nullptr },
    { "xor", OP_XOR, true, false, { reg_val, reg_reg_val }, false, transform::duplicate_reg },
    { "zero", 0x00, true, false, { { ArgumentType::Register } }, false, transform::zero },
  };
}
