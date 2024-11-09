#include <processor/src/constants.h>

#include "instruction.hpp"
#include "extra.hpp"
#include "signature.hpp"
#include "util.hpp"

namespace assembler::instruction {
    std::unordered_map<std::string, uint8_t> conditional_postfix_map = {
            {"z",   CMP_Z},
            {"nz",  CMP_NZ},
            {"neq", CMP_NE},
            {"ne",  CMP_NE},
            {"eq",  CMP_EQ},
            {"lte", CMP_LE},
            {"lt",  CMP_LT},
            {"le",  CMP_LE},
            {"gte", CMP_GE},
            {"gt",  CMP_GT},
            {"ge",  CMP_GE},
    };

    std::map<std::string, uint8_t> datatype_postfix_map = {
            {"hu", DATATYPE_U32},
            {"u",  DATATYPE_U64},
            {"hi", DATATYPE_S32},
            {"i",  DATATYPE_S64},
            {"f",  DATATYPE_F},
            {"d",  DATATYPE_D}
    };

    Signature *find_signature(const std::string &mnemonic, std::string &options) {
        for (auto &signature: signature_list) {
            if (starts_with(mnemonic, signature.mnemonic)) {
                options = mnemonic.substr(signature.mnemonic.size());
                return &signature;
            }
        }

        return nullptr;
    }

    Signature *find_signature(const std::string &mnemonic) {
        for (auto &signature: signature_list) {
            if (starts_with(mnemonic, signature.mnemonic)) {
                return &signature;
            }
        }

        return nullptr;
    }

    const std::deque reg_val = {ArgumentType::Register, ArgumentType::Value};
    const std::deque reg_reg_val = {ArgumentType::Register, ArgumentType::Register, ArgumentType::Value};

    const Signature
            Signature::_add = {"add", OP_ADD, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_and = {"and", OP_AND, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
//    Signature::_call = { "call", OP_CALL, true, false, { { ArgumentType::Address } }, false, nullptr },
    Signature::_cvt = {"cvt", OP_CONVERT, true, false,
                       {{ArgumentType::Register}, {ArgumentType::Register, ArgumentType::Register}}, false,
                       parse::convert, transform::transform_reg_reg},
            Signature::_div = {"div", OP_DIV, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_jal = {"jal", OP_JAL, true, false, {{ArgumentType::Value}, reg_val}, false, nullptr,
                               transform::transform_jal},
            Signature::_load = {"load", OP_LOAD, true, false, {reg_val}, false, nullptr, nullptr},
            Signature::_loadu = {"loadu", OP_LOAD_UPPER, true, false, {reg_val}, false, nullptr, nullptr},
            Signature::_mod = {"mod", OP_MOD, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_mul = {"mul", OP_MUL, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_nop = {"nop", OP_NOP, false, false, {{}}, false, nullptr, nullptr},
            Signature::_not = {"not", OP_NOT, true, false,
                               {{ArgumentType::Register}, {ArgumentType::Register, ArgumentType::Register}}, false,
                               nullptr, transform::transform_reg_reg},
            Signature::_or = {"or", OP_OR, true, false, {reg_val, reg_reg_val}, false, nullptr,
                              transform::transform_reg_reg_val},
            Signature::_push = {"push", OP_PUSH, true, false, {{ArgumentType::Value}}, false, nullptr, nullptr},
//    Signature::_ret = { "ret", OP_RET, true, false, { { } }, false, nullptr, nullptr },
    Signature::_shl = {"shl", OP_SHL, true, false, {reg_val, reg_reg_val}, false, nullptr,
                       transform::transform_reg_reg_val},
            Signature::_shr = {"shr", OP_SHR, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_store = {"store", OP_STORE, true, false, {{ArgumentType::Register, ArgumentType::Address}},
                                 false, nullptr, nullptr},
            Signature::_sub = {"sub", OP_SUB, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_syscall = {"syscall", OP_SYSCALL, true, false, {{ArgumentType::Value}}, false, nullptr,
                                   nullptr},
            Signature::_xor = {"xor", OP_XOR, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val};

    std::vector<Signature> signature_list = {
            Signature::_add,
            Signature::_and,
            {"b", 0x00, true, false, {{ArgumentType::Value}}, false, nullptr, transform::branch},
            Signature::_cvt,
            Signature::_div,
            {"exit", 0x00, true, false, {{}, {ArgumentType::Value}}, false, nullptr, transform::exit},
            {"int", 0x00, true, false, {{ArgumentType::Value}}, false, nullptr, transform::interrupt},
            Signature::_jal,
            {"jmp", 0x00, false, false, {{ArgumentType::Value}}, false, nullptr, transform::jump},
            Signature::_loadu,
            {"loadi", 0x00, true, false, {{ArgumentType::Register, ArgumentType::Immediate}}, true, nullptr,
             transform::load_immediate},
            Signature::_load,
            Signature::_mod,
            Signature::_mul,
            Signature::_nop,
            Signature::_not,
            Signature::_or,
            Signature::_push,
            {"rti", 0x00, true, false, {{}}, false, nullptr, transform::interrupt_return},
            Signature::_shl,
            Signature::_shr,
            Signature::_store,
            Signature::_sub,
            Signature::_syscall,
            Signature::_xor,
            {"zero", 0x00, true, false, {{ArgumentType::Register}}, false, nullptr, transform::zero},
    };

}
