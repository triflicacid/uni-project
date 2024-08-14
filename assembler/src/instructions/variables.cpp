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

  std::vector<Signature> signature_list = {
    { "loadu", OP_LOAD_UPPER, true, false, { ArgumentType::Register, ArgumentType::Value }, false, nullptr },
    { "loadw", 0x00, true, false, { ArgumentType::Register, ArgumentType::Value }, true, transform::loadw },
    { "load", OP_LOAD, true, false, { ArgumentType::Register, ArgumentType::Value }, false, nullptr },
    { "nop", OP_NOP, false, false, { }, false, nullptr },
    { "store", OP_STORE, true, false, { ArgumentType::Register, ArgumentType::Address }, false, nullptr },
    { "syscall", OP_SYSCALL, true, false, { ArgumentType::Value }, false, nullptr },
    { "zero", 0x00, true, false, { ArgumentType::Register }, false, transform::zero },
  };
}
