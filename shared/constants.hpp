#pragma once

#include "shell.hpp"
#include <cstdint>
#include <optional>
#include <map>
#include <string>
#include <unordered_map>

namespace constants {
    // default address of the interrupt handler
    constexpr uint64_t default_interrupt_handler = 0x400;

    // mask for cmp bits (@pos 0)
    constexpr uint64_t cmp_bits = 0x7;

    // halt on nop instruction?
    constexpr bool halt_on_nop = true;

    namespace registers {
        constexpr uint8_t count = 32;

        enum reg : uint8_t {
            pc = 0,
            rpc,
            sp,
            fp,
            flag,
            isr,
            imr,
            ipc,
            ret,
            k1,
            k2,
            r1, // start of general registers
        };

        extern std::map<std::string, constants::registers::reg> map;

        std::string to_string(reg r);

        std::optional<reg> from_string(const std::string &s);
        std::optional<reg> from_string(const std::string &s, int &i);
    }

    enum class flag {
        zero = 0x8,
        is_running = 0x10,
        in_interrupt = 0x100,
        error = 0xe0,
    };

    namespace cmp {
        enum flag : uint8_t {
            ne = 0b0000,
            eq = 0b0001,
            lt = 0b0010,
            le = 0b0011,
            nz = 0b1000,
            gt = 0b0110,
            ge = 0b0111,
            z = 0b1001,
            na = 0b1111,
        };

        extern std::unordered_map<std::string, flag> map;

        std::string to_string(flag v);

        std::optional<flag> from_string(const std::string &s);
        std::optional<flag> from_string(const std::string &s, int &i);
    }

    namespace error {
        constexpr uint8_t offset = 5;
        constexpr uint32_t mask = 0x7;

        enum code {
            ok = 0b000,
            opcode = 0b001,
            segfault = 0b010,
            reg = 0b011,
            syscall = 0b100,
            datatype = 0b101,
            unknown = 0b111,
        };
    }

    enum class syscall {
        print_hex,
        print_int,
        print_float,
        print_double,
        print_char,
        print_string,
        read_int,
        read_float,
        read_double,
        read_char,
        read_string,
        exit,
        print_regs = 100,
        print_mem,
        print_stack
    };

    namespace inst {
        constexpr uint32_t op_mask = 0x3f;
        constexpr uint8_t op_size = 6;

        constexpr uint64_t test_bit = 0x40;
        constexpr uint64_t cmp_offset = 6;
        constexpr uint64_t cmp_mask = 0xf;
        constexpr uint8_t cmp_size = 4;

        constexpr uint8_t header_size = 10;

        constexpr uint8_t reg_size = 8;
        constexpr uint8_t value_size = 34;
        constexpr uint8_t addr_size = 33;

        enum arg {
            imm = 0b00,
            reg = 0b01,
            mem = 0b10,
            reg_indirect = 0b11,
        };

        namespace datatype {
            constexpr uint8_t size = 3;

            enum dt {
                u32 = 0b000,
                u64 = 0b001,
                s32 = 0b010,
                s64 = 0b011,
                flt = 0b100,
                dbl = 0b101,
            };

            extern std::unordered_map<std::string, dt> map;

            std::string to_string(dt v);

            std::optional<dt> from_string(const std::string &s);
            std::optional<dt> from_string(const std::string &s, int &i);
        }

        enum op {
            _nop = 0x00,
            _load,
            _load_upper,
            _store,
            _compare,
            _convert,
            _not,
            _and,
            _or,
            _xor,
            _shr,
            _shl,
            _zext,
            _sext,
            _add,
            _sub,
            _mul,
            _div,
            _mod,
            _jal,
            _push, // deprecated
            _syscall = 0x3f,
        };
    }
}
