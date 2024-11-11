#include "extra.hpp"
#include "signature.hpp"

#include <util.hpp>
#include <processor/src/constants.h>

namespace assembler::instruction::transform {
    void
    transform_reg_reg(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                      int overload) {
        if (instruction->args.size() == 1) {
            // duplicate <reg>
            instruction->args.emplace_back(instruction->args[0]);
            instruction->overload++;
        }

        instructions.push_back(std::move(instruction));
    }

    void transform_reg_reg_val(std::vector<std::unique_ptr<Instruction>> &instructions,
                               std::unique_ptr<Instruction> instruction, int overload) {
        if (instruction->args.size() == 2) {
            // duplicate <reg>
            instruction->args.emplace_front(instruction->args[0]);
            instruction->overload++;
        }

        instructions.push_back(std::move(instruction));
    }

    void
    transform_jal(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                  int overload) {
        if (instruction->args.size() == 1) {
            // add $rip as register
            instruction->args.emplace_front(ArgumentType::Register, REG_RIP);
            instruction->overload++;
        }

        instructions.push_back(std::move(instruction));
    }

    void branch(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                int overload) {
        // original: "b $addr"
        // "load $ip, $addr"
        instruction->signature = &Signature::_load;
        instruction->overload = 0;
        instruction->args.emplace_front(ArgumentType::Register, REG_IP);
        instructions.push_back(std::move(instruction));
    }

    void exit(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload) {
        // original: "exit [code]"
        // extract code?
        uint64_t code = 0;
        if (overload) {
            code = instruction->args[0].get_data();
            instruction->args.pop_back();
        }

        // if provided, load code into $ret
        if (overload) {
            auto code_instruction = std::make_unique<Instruction>(*instruction);
            code_instruction->signature = &Signature::_load;
            code_instruction->overload = 0;
            code_instruction->args.emplace_back(ArgumentType::Register, REG_RET);
            code_instruction->args.emplace_back(ArgumentType::Immediate, code);
            instructions.push_back(std::move(code_instruction));
        }

        // "syscall <opcode: exit>"
        instruction->signature = &Signature::_syscall;
        instruction->overload = 0;
        instruction->args.emplace_back(ArgumentType::Immediate, SYSCALL_EXIT);
        instructions.push_back(std::move(instruction));
    }

    void interrupt(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload) {
        // original: "int <value>"
        // "or $isr, <value>"
        instruction->signature = &Signature::_or;
        instruction->overload = 1;
        instruction->args.emplace_front(ArgumentType::Register, REG_ISR);
        instruction->args.emplace_front(ArgumentType::Register, REG_ISR);
        instructions.push_back(std::move(instruction));
    }

    void
    interrupt_return(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                     int overload) {
        // original: "rti"
        // "load $ip, $iip"
        instruction->signature = &Signature::_load;
        instruction->overload = 0;
        instruction->args.emplace_back(ArgumentType::Register, REG_IP);
        instruction->args.emplace_back(ArgumentType::Register, REG_IIP);
        instructions.push_back(std::move(instruction));

        // "and $flag, ~<in interrupt>"
        instruction = std::make_unique<Instruction>(*instruction);
        instruction->signature = &Signature::_and;
        instruction->overload = 1;
        instruction->args[0].update(ArgumentType::Register, REG_FLAG);
        instruction->args[1].update(ArgumentType::Register, REG_FLAG);
        instruction->args.emplace_back(ArgumentType::Immediate, ~FLAG_IN_INTERRUPT);
        instructions.push_back(std::move(instruction));
    }

    void jump(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload) {
        branch(instructions, std::move(instruction), overload);
    }

    void
    load_immediate(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload) {
        // original: "loadi $r, $i"
        uint64_t imm = instruction->args[1].get_data();

        // "load $r, $i[:32]"
        instruction->signature = &Signature::_load;
        instruction->overload = 0;
        instruction->args[1].update(ArgumentType::Immediate, imm & 0xffffffff);
        instructions.push_back(std::move(instruction));

        // "loadu $r, $i[32:]"
        instruction = std::make_unique<Instruction>(*instruction);
        instruction->signature = &Signature::_loadu;
        instruction->overload = 0;
        instruction->args[1].set_data(imm >> 32);
        instructions.push_back(std::move(instruction));
    }

    void zero(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload) {
        // original: "zero $r"
        // "load $r, 0"
        instruction->signature = &Signature::_load;
        instruction->overload = 0;
        instruction->args.emplace_back(ArgumentType::Immediate, 0);
        instructions.push_back(std::move(instruction));
    }
}

namespace assembler::instruction::parse {
    void
    convert(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction, std::string &options,
            message::List &msgs) {
        int &col = loc.columnref();

        for (uint8_t i = 0; i < 2; i++) {
            // parse datatype
            bool found = false;

            for (auto &pair: datatype_postfix_map) {
                if (starts_with(options, pair.first)) {
                    found = true;
                    instruction->add_datatype_specifier(pair.second);

                    // increase position
                    options = options.substr(pair.first.length());

                    // if first datatype, expect '2'
                    if (i == 0) {
                        if (options[0] == '2') {
                            options = options.substr(1);
                        } else {
                            auto msg = std::make_unique<message::Message>(message::Error, loc);
                            msg->get() << "cvt: expected '2' after first datatype, got '" << options[0] << "'";
                            msgs.add(std::move(msg));
                            return;
                        }
                    }

                    break;
                }
            }

            if (!found) {
                auto msg = std::make_unique<message::Message>(message::Error, loc);
                msg->get() << "cvt: expected datatype. Syntax: cvt(d1)2(d2)";
                msgs.add(std::move(msg));
                return;
            }
        }
    }
}
