#include "instruction.hpp"
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

  std::map<std::string, uint8_t> opcode_map = {

  };

  uint8_t *find_opcode(const std::string &mnemonic, std::string &options) {
    for (auto &pair : opcode_map) {
      if (starts_with(mnemonic, pair.first)) {
        options = mnemonic.substr(pair.first.size());
        return &pair.second;
      }
    }

    auto signature = find_signature(mnemonic, options);
    return signature == nullptr ? nullptr : &signature->opcode;
  }

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
    { "loadu", OP_LOAD_UPPER, true, false, { ArgumentType::Register, ArgumentType::Value } },
    { "loadw", 0x00, true, false, { ArgumentType::Register, ArgumentType::Value } },
    { "load", OP_LOAD, true, false, { ArgumentType::Register, ArgumentType::Value } },
    { "nop", OP_NOP, false, false, { } },
    { "store", OP_STORE, true, false, { ArgumentType::Register, ArgumentType::Address } },
    { "syscall", OP_SYSCALL, true, false, { ArgumentType::Value } },
    { "zero", 0x00, true, false, { ArgumentType::Register } },
  };
}
