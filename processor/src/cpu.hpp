#pragma once

#include <ostream>
#include <functional>
#include "bus.hpp"
#include "constants.hpp"
#include "debug.hpp"
#include "core.hpp"

namespace processor {
    class CPU : public Core {
        uint64_t addr_interrupt_handler{};

        [[nodiscard]] static bool flag_test(uint64_t bitstr, constants::flag v) { return bitstr & int(v); }
        [[nodiscard]] bool flag_test(constants::flag v, bool silent = false) const { return reg(constants::registers::flag, silent) & int(v); }
        void flag_set(constants::flag v, bool silent = false) { reg_set(constants::registers::flag, reg(constants::registers::flag, silent) | int(v), silent); }
        void flag_reset(constants::flag v) {
            reg_set(constants::registers::flag, reg(constants::registers::flag) & ~int(v)); }
        void halt() { flag_reset(constants::flag::is_running); }

        template<typename T>
        void push(T val);

        // set the zero flag based on contents of the register
        void test_is_zero(constants::registers::reg reg);

        // extract `<reg>` argument from data
        constants::registers::reg _arg_reg(uint32_t data);

        // extract `<addr>` argument from data
        uint32_t _arg_addr(uint32_t data);

        // extract `<register indirect>` address argument from word, return offset
        uint32_t _arg_reg_indirect(uint32_t data);

        // get argument `<reg>`
        [[nodiscard]] constants::registers::reg get_arg_reg(uint64_t inst, uint8_t pos);

        // extract `<value>` argument from word, fetch value
        // cast_imm_double: if true, cast imm to double (as 32-bit imm only, so float)
        [[nodiscard]] uint64_t get_arg_value(uint64_t word, uint8_t pos, bool cast_imm_double);

        // extract `<addr>` argument from word (returns address, doesn't extract value)
        [[nodiscard]] uint64_t get_arg_addr(uint64_t word, uint8_t pos);

        // fetch `<reg> <reg> <value>`, return if OK. Index into instruction `HEADER_SIZE + offset`
        // pass `is_double` to `get_arg` of value
        [[nodiscard]] bool fetch_reg_reg_val(uint64_t inst, constants::registers::reg &reg1, constants::registers::reg &reg2, uint64_t &value, uint8_t offset, bool is_double);

        // opcode execution instructions
        void exec_load(uint64_t inst);
        void exec_load_upper(uint64_t inst);
        void exec_store(uint64_t inst);
        void exec_compare(uint64_t inst);
        void exec_convert(uint64_t inst);
        void exec_not(uint64_t inst);
        void exec_and(uint64_t inst);
        void exec_or(uint64_t inst);
        void exec_xor(uint64_t inst);
        void exec_shift_left(uint64_t inst);
        void exec_shift_right(uint64_t inst);
        void exec_zero_extend(uint64_t inst);
        void exec_sign_extend(uint64_t inst);
        void exec_add(uint64_t inst);
        void exec_sub(uint64_t inst);
        void exec_mul(uint64_t inst);
        void exec_div(uint64_t inst);
        void exec_mod(uint64_t inst);
        void exec_jal(uint64_t inst);
        void exec_push(uint64_t inst);
        void exec_syscall(uint64_t inst);

    public:
        CPU() : Core(), addr_interrupt_handler(constants::default_interrupt_handler) {}

        void set_interrupt_handler(uint64_t addr) { addr_interrupt_handler = addr; }

        // sets $ip, use carefully while running
        void jump(uint64_t val) { reg_set(constants::registers::ip, val); }

        // read $ret
        [[nodiscard]] uint64_t get_return_value() const { return reg(constants::registers::ret); }

        [[nodiscard]] bool is_running() const { return flag_test(constants::flag::is_running); }

        [[nodiscard]] constants::error::code get_error() const { return static_cast<constants::error::code>(
                    (reg(constants::registers::flag) >> constants::error::offset) & constants::error::mask); }

        // halt cpu, set error code bits, $ret=val
        void raise_error(constants::error::code code, uint64_t val);

        // same as raise_error, but returns ret from function
        uint64_t raise_error(constants::error::code code, uint64_t val, uint64_t ret);

        // test if there is an interrupt THAT IS NOT BEING HANDLED
        [[nodiscard]] bool is_interrupt() const;

        // handle interrupt - jump to handler
        // note, does not check $imr or $isr
        void handle_interrupt();

        // fetch next instruction, DO NOT increment $ip
        [[nodiscard]] uint64_t fetch();

        // execute the given instruction
        void execute(uint64_t inst);

        // execute a single step in the fetch-execute cycle
        // argument `step` is for debug output only
        void step(int step = -1);

        // run the fetch-execute cycle (call step() until halt)
        void step_cycle();

        // print error details (doesn't print if no error)
        void print_error(bool prefix) const;

        // check if the given address is valid
        [[nodiscard]] static bool check_memory(uint64_t addr) { return addr < dram::size; }

        // check if the given register is valid
        [[nodiscard]] static bool check_register(uint8_t off) { return off < constants::registers::count; }
    };
}