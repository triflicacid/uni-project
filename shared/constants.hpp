#pragma once

#include "shell.hpp"
#include <cstdint>
#include <optional>
#include <map>
#include <string>
#include <unordered_map>

/** @brief Constants and encodings shared by the processor, assembler, and compiler: register indices, flag bits, instruction encoding, and opcodes. */
namespace constants {
    /** @brief Default address the interrupt handler starts at. */
    constexpr uint64_t default_interrupt_handler = 0x400;

    /** @brief Mask for the cmp bits within the flag register (bit position 0). */
    constexpr uint64_t cmp_bits = 0x7;

    /** @brief Whether execution should halt when a nop instruction is reached. */
    extern bool halt_on_nop;

    /** @brief Register indices and lookup/description helpers. */
    namespace registers {
        /** @brief Total number of registers. */
        constexpr uint8_t count = 32;

        /** @brief Index of each register in the register file. */
        enum reg : uint8_t {
            pc = 0, ///< Program counter, points to the next instruction word in memory.
            rpc, ///< Return program counter, set to the current $pc on call to a procedure, jumped back to when the procedure is exited.
            sp, ///< Stack pointer, points to the top of the stack (specifically, the byte above the top of the stack).
            fp, ///< Frame pointer, points to the top of the latest stack frame.
            flag, ///< Contains various state information about the processor.
            isr, ///< Interrupt status register; a set bit indicates that interrupt is pending.
            imr, ///< Interrupt mask register; a bitmask used to 'cancel out' or ignore pending interrupts in $isr.
            ipc, ///< Interrupt program counter, set to $pc on an interrupt, used to restore it on return from the interrupt handler.
            ret, ///< Return register, stores the return value from the procedure/process, or error information on error.
            k1, ///< Internal register used by pseudo-instructions and as a scratch register for the interrupt handler.
            k2, ///< Internal register used by pseudo-instructions and as a scratch register for the interrupt handler.
            r1, ///< Start of the general-purpose registers, free for the programmer's use.
        };

        /** @brief Maps a register's mnemonic name to its @ref reg index. */
        extern std::map<std::string, constants::registers::reg> map;

        /**
         * @brief Get the mnemonic name of a register.
         * @param r Register to name.
         * @return The register's name (e.g. "pc"), or the general-purpose form (e.g. "r3") if not one of the named registers.
         */
        std::string to_string(reg r);

        /**
         * @brief Parse a register from its full mnemonic name.
         * @param s String expected to hold exactly a register name.
         * @return The parsed register, or empty if `s` isn't a valid register name.
         */
        std::optional<reg> from_string(const std::string &s);
        /**
         * @brief Parse a register from a prefix of `s`, advancing past what was consumed.
         * @param s String to parse from.
         * @param i Index to start parsing at; advanced past the consumed register name on success.
         * @return The parsed register, or empty if no valid register name starts at `i`.
         */
        std::optional<reg> from_string(const std::string &s, int &i);

        /**
         * @brief Get a human-readable description of a register's purpose.
         * @param r Register to describe.
         * @return Description text, or empty if `r` has no special-purpose description.
         */
        std::optional<std::string> describe(reg r);

        /** @brief First register that syscall arguments are pushed to. */
        extern reg syscall_start;
    }

    /** @brief Bit flags held in the $flag register. */
    enum class flag {
        zero = 0x8, ///< Set when the last data-affecting instruction produced a zero result.
        is_running = 0x10, ///< Set while the processor is running; cleared to halt execution.
        in_interrupt = 0x100, ///< Set while an interrupt handler is executing.
        error = 0xe0, ///< Mask covering the error-code bits, set by @ref error::code.
    };

    /** @brief Comparison result flags produced by the compare instruction and tested by conditional jumps. */
    namespace cmp {
        /** @brief A comparison condition (equality/ordering), encoded as a bitmask of a base condition plus an optional 'inverse' bit. */
        enum flag : uint8_t {
            z  = 0b1000, ///< Zero flag is set (mnemonic `z`); also the shared base bit set by eq/lt/gt.
            eq = 0b1010, ///< Equal.
            lt = 0b1001, ///< Less than.
            gt = 0b1011, ///< Greater than.

            nz  = 0b100 | z, ///< Not zero.
            neq = 0b100 | eq, ///< Not equal.
            nlt = 0b100 | lt, ///< Not less than.
            ge = nlt, ///< Greater than or equal to (alias of @ref nlt).
            ngt = 0b100 | gt, ///< Not greater than.
            le = ngt, ///< Less than or equal to (alias of @ref ngt).

            na = 0b0000, ///< No condition; always passes.
        };

        /** @brief Maps a cmp flag's mnemonic name to its @ref flag value. */
        extern std::unordered_map<std::string, flag> map;

        /**
         * @brief Get the mnemonic name of a cmp flag.
         * @param v Flag to name.
         * @return The flag's name, or "?" if `v` has no known name.
         */
        std::string to_string(flag v);

        /**
         * @brief Parse a cmp flag from its full mnemonic name.
         * @param s String expected to hold exactly a cmp flag name.
         * @return The parsed flag, or empty if `s` isn't a valid cmp flag name.
         */
        std::optional<flag> from_string(const std::string &s);
        /**
         * @brief Parse a cmp flag from a prefix of `s`, advancing past what was consumed.
         * @param s String to parse from.
         * @param i Index to start parsing at; advanced past the consumed flag name on success.
         * @return The parsed flag, or empty if no valid flag name starts at `i`.
         */
        std::optional<flag> from_string(const std::string &s, int &i);

        /**
         * @brief Get the logical inverse of a cmp flag (e.g. `eq` <-> `neq`).
         * @param input Flag to invert.
         * @return The inverted flag.
         */
        flag inverse_of(flag input);
    }

    /** @brief Error codes reported in the $ret register when a syscall or instruction faults. */
    namespace error {
        /** @brief Bit offset of the error code within the $flag register. */
        constexpr uint8_t offset = 5;
        /** @brief Mask for the error code once shifted to bit 0. */
        constexpr uint32_t mask = 0x7;

        /** @brief Specific error condition that occurred. */
        enum code {
            ok = 0b000, ///< No error.
            opcode = 0b001, ///< Unrecognised or invalid opcode.
            segfault = 0b010, ///< Out-of-bounds or otherwise invalid memory access.
            reg = 0b011, ///< Invalid register index.
            syscall = 0b100, ///< Unrecognised or invalid syscall number.
            datatype = 0b101, ///< Invalid datatype bit pattern.
            unknown = 0b111, ///< Unspecified error.
        };
    }

    /** @brief Identifiers for the syscalls the processor supports. */
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
        copy_mem,
        print_regs = 100,
        print_mem,
        print_stack
    };

    /** @brief Bit-level layout constants for encoded instruction words, and the opcode/argument/datatype enums used to interpret them. */
    namespace inst {
        /** @brief Mask for the opcode field once shifted to bit 0. */
        constexpr uint32_t op_mask = 0x3f;
        /** @brief Width in bits of the opcode field. */
        constexpr uint8_t op_size = 6;

        /** @brief Bit that marks an instruction as a conditional (test) instruction. */
        constexpr uint64_t test_bit = 0x40;
        /** @brief Bit offset of the cmp-condition field within an instruction word. */
        constexpr uint64_t cmp_offset = 6;
        /** @brief Mask for the cmp-condition field once shifted to bit 0. */
        constexpr uint64_t cmp_mask = 0xf;
        /** @brief Width in bits of the cmp-condition field. */
        constexpr uint8_t cmp_size = 4;

        /** @brief Width in bits of an instruction's fixed header (opcode + test bit + cmp condition + datatype). */
        constexpr uint8_t header_size = 10;

        /** @brief Width in bits of a register-index argument. */
        constexpr uint8_t reg_size = 8;
        /** @brief Width in bits of an immediate-value argument. */
        constexpr uint8_t value_size = 34;
        /** @brief Width in bits of an address argument. */
        constexpr uint8_t addr_size = 33;

        /** @brief Kind of argument an instruction operand encodes. */
        enum arg {
            imm = 0b00,
            reg = 0b01,
            mem = 0b10,
            reg_indirect = 0b11,
        };

        /**
         * @brief Get a human-readable name for an argument kind.
         * @param a Argument kind to name.
         * @return Descriptive name (e.g. "register").
         */
        std::string arg_to_string(arg a);

        /** @brief Datatype tag encoded in an instruction, selecting how operands' bits should be interpreted. */
        namespace datatype {
            /** @brief Width in bits of the datatype field. */
            constexpr uint8_t size = 3;

            /** @brief Concrete datatype an instruction operates on. */
            enum dt {
                u32 = 0b000, ///< `uint32`.
                u64 = 0b001, ///< `uint64`.
                s32 = 0b010, ///< `int32`.
                s64 = 0b011, ///< `int64`.
                flt = 0b100, ///< `float32`.
                dbl = 0b101, ///< `float64`.
            };

            /** @brief Maps a datatype's mnemonic suffix to its @ref dt value. */
            extern std::unordered_map<std::string, dt> map;

            /**
             * @brief Get the mnemonic suffix of a datatype.
             * @param v Datatype to name.
             * @return The datatype's mnemonic suffix (e.g. "f"), or "?" if `v` has no known name.
             */
            std::string to_string(dt v);

            /**
             * @brief Parse a datatype from its full mnemonic suffix.
             * @param s String expected to hold exactly a datatype suffix.
             * @return The parsed datatype, or empty if `s` isn't a valid suffix.
             */
            std::optional<dt> from_string(const std::string &s);
            /**
             * @brief Parse a datatype from a prefix of `s`, advancing past what was consumed.
             * @param s String to parse from.
             * @param i Index to start parsing at; advanced past the consumed suffix on success.
             * @return The parsed datatype, or empty if no valid suffix starts at `i`.
             */
            std::optional<dt> from_string(const std::string &s, int &i);
        }

        /** @brief Instruction opcode. */
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
            _push, ///< Deprecated.
            _syscall = 0x3f,
        };

        /**
         * @brief Get the assembly mnemonic for an opcode.
         * @param opcode Opcode value to name.
         * @return The mnemonic (e.g. "add"), or "?" if `opcode` is not a known opcode.
         */
        std::string opcode_to_mnemonic(int opcode);
    }
}

/**
 * @brief Cast an integer to a register index.
 * @param i Integer register index.
 * @return `i` reinterpreted as a @ref constants::registers::reg.
 */
inline constants::registers::reg $reg(int i) {
  return static_cast<constants::registers::reg>(i);
}
