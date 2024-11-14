#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include "constants.hpp"
#include "bus.hpp"
#include "debug.hpp"

namespace processor {
    class Core {
        std::array<uint64_t, constants::registers::count> m_regs{}; // register store
        bus m_bus{}; // connected bus to access memory

    protected:
        [[nodiscard]] uint64_t reg(constants::registers::reg r, bool silent = false) const;

        template<typename T>
        [[nodiscard]] T reg(constants::registers::reg r, bool silent = false) const {
            if (debug::reg && !silent) std::cout << DEBUG_STR ANSI_BRIGHT_YELLOW " reg_copy" ANSI_RESET ": access $" << constants::registers::to_string(r) << " -> 0x" << std::hex << m_regs[r] << std::dec << std::endl;
            return *(T*) &m_regs[r];
        }

        void reg_set(constants::registers::reg r, uint64_t val, bool silent = false);
        void reg_copy(constants::registers::reg rd, constants::registers::reg rs, bool silent = false);
        void reg_upper(constants::registers::reg r, uint32_t val) { *(uint32_t *) &m_regs[r] = val; }

        [[nodiscard]] uint64_t mem_load(uint64_t addr, uint8_t size);
        void mem_store(uint64_t addr, uint8_t size, uint64_t data);

        // read a string of size `length` from the input stream and write into memory at `addr`
        void read_string(uint64_t addr, uint32_t length);

        // write a null-terminated C-string, starting at `addr`, to the output stream
        void write_string(uint64_t addr);

    public:
        Core(std::ostream &os, std::istream &is);

        std::ostream &os;
        std::istream &is;

        // print contents of stack as hexadecimal bytes
        void print_stack() const;

        // print contents of all registers as hexadecimal
        void print_registers() const;

        // print region of memory, assume locations are valid
        void print_memory(uint64_t addr, uint32_t bytes);

        // read data from the input stream into memory (starting at address 0x0)
        void read(std::fstream &is, size_t bytes);
    };

    // check if the given address is valid
    bool check_memory(uint64_t addr);

    // check if the given register is valid
    bool check_register(uint8_t off);
}
