#include "constants.hpp"

#include "instruction.hpp"
#include "extra.hpp"
#include "signature.hpp"
#include "util.hpp"

namespace assembler::instruction {
    using namespace constants;

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
            Signature::_add = {"add", inst::_add, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_and = {"and", inst::_and, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
//    Signature::_call = { "call", inst::_call, true, false, { { ArgumentType::Address } }, false, nullptr },
    Signature::_cvt = {"cvt", inst::_convert, true, false,
                       {{ArgumentType::Register}, {ArgumentType::Register, ArgumentType::Register}}, false,
                       parse::convert, transform::transform_reg_reg},
            Signature::_div = {"div", inst::_div, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_jal = {"jal", inst::_jal, true, false, {{ArgumentType::Value}, reg_val}, false, nullptr,
                               transform::transform_jal},
            Signature::_load = {"load", inst::_load, true, false, {reg_val}, false, nullptr, nullptr},
            Signature::_loadu = {"loadu", inst::_load_upper, true, false, {reg_val}, false, nullptr, nullptr},
            Signature::_mod = {"mod", inst::_mod, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_mul = {"mul", inst::_mul, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_nop = {"nop", inst::_nop, false, false, {{}}, false, nullptr, nullptr},
            Signature::_not = {"not", inst::_not, true, false,
                               {{ArgumentType::Register}, {ArgumentType::Register, ArgumentType::Register}}, false,
                               nullptr, transform::transform_reg_reg},
            Signature::_or = {"or", inst::_or, true, false, {reg_val, reg_reg_val}, false, nullptr,
                              transform::transform_reg_reg_val},
            Signature::_push = {"push", inst::_push, true, false, {{ArgumentType::Value}}, false, nullptr, nullptr},
//    Signature::_ret = { "ret", inst::_ret, true, false, { { } }, false, nullptr, nullptr },
    Signature::_shl = {"shl", inst::_shl, true, false, {reg_val, reg_reg_val}, false, nullptr,
                       transform::transform_reg_reg_val},
            Signature::_shr = {"shr", inst::_shr, true, false, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_store = {"store", inst::_store, true, false, {{ArgumentType::Register, ArgumentType::Address}},
                                 false, nullptr, nullptr},
            Signature::_sub = {"sub", inst::_sub, true, true, {reg_val, reg_reg_val}, false, nullptr,
                               transform::transform_reg_reg_val},
            Signature::_syscall = {"syscall", inst::_syscall, true, false, {{ArgumentType::Value}}, false, nullptr,
                                   nullptr},
            Signature::_xor = {"xor", inst::_xor, true, false, {reg_val, reg_reg_val}, false, nullptr,
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
