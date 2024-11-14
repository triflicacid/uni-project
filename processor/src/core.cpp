#include "core.hpp"
#include "debug.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>

processor::Core::Core(std::ostream &os, std::istream &is) : os(os), is(is) {
    using namespace constants;

    // clear and configure key registers
    memset(m_regs.data(), 0, sizeof(m_regs));
    reg_set(registers::imr, 0xffffffffffffffff);
    reg_set(registers::sp, dram::size);
    reg_copy(registers::fp, registers::sp);

    // clear memory
    m_bus.mem.clear();
}

uint64_t processor::Core::reg(constants::registers::reg r, bool silent) const {
    if (debug::reg && !silent) std::cout << DEBUG_STR ANSI_BRIGHT_YELLOW " reg" ANSI_RESET ": access $" << constants::registers::to_string(r) << " -> 0x" << std::hex << m_regs[r] << std::dec << std::endl;
    return m_regs[r];
}

void processor::Core::reg_set(constants::registers::reg r, uint64_t val, bool silent) {
    if (debug::reg && !silent) std::cout << DEBUG_STR ANSI_BRIGHT_YELLOW " reg" ANSI_RESET ": set $" << constants::registers::to_string(r) << " to 0x" << std::hex << val << std::dec << std::endl;
    m_regs[r] = val;
}

void processor::Core::reg_copy(constants::registers::reg rd, constants::registers::reg rs, bool silent) {
    if (debug::reg && !silent) std::cout << DEBUG_STR ANSI_BRIGHT_YELLOW " reg" ANSI_RESET ": copy $" << constants::registers::to_string(rs) << " into $" << constants::registers::to_string(rd) << " -> 0x" << std::hex << m_regs[rs] << std::dec << std::endl;
    m_regs[rd] = m_regs[rs];
}

uint64_t processor::Core::mem_load(uint64_t addr, uint8_t size) {
    if (debug::mem) std::cout << DEBUG_STR ANSI_YELLOW " mem" ANSI_RESET ": access " << (int) size << " bytes from address 0x" << std::hex << addr << " -> ";
    uint64_t data = m_bus.load(addr, size);
    if (debug::mem) std::cout << "0x" << data << std::dec << std::endl;
    return data;
}

void processor::Core::mem_store(uint64_t addr, uint8_t size, uint64_t data) {
    if (debug::mem) std::cout << DEBUG_STR ANSI_YELLOW " mem" ANSI_RESET ": store data 0x" << std::hex << data << " of " << std::dec << size << " bytes at address 0x" << std::hex << addr << std::dec << std::endl;
    m_bus.store(addr, size, data);
}

void processor::Core::read(std::fstream &stream, size_t bytes) {
    stream.read((char *) m_bus.mem.data(), bytes);
}

void processor::Core::read_string(uint64_t addr, uint32_t length) {
    is.read((char *) (m_bus.mem.data() + addr), length);
}

void processor::Core::write_string(uint64_t addr) {
    os << (const char *) (m_bus.mem.data() + addr);
}

void processor::Core::print_registers() const {
    using namespace constants::registers;
    uint8_t i;
    os << std::hex;

    // special registers
    for (i = 0; i < r1; i++) {
        os << std::left << std::setw(8) << constants::registers::to_string(static_cast<enum reg>(i)) << " = 0x" << reg(static_cast<enum reg>(i)) << std::endl;
    }

    // general registers
    for (i = 0; i < count - r1; i++) {
        os << "$r" << i + 1 << "      = 0x" << reg(static_cast<enum reg>(r1 + i)) << std::endl;
    }

    os << std::dec;
}

void processor::Core::print_stack() const {
    uint64_t addr = reg(constants::registers::sp), size = dram::size - addr;
    os << "STACK: top = 0x" << std::hex << dram::size - 1 << " -> bottom = 0x" << addr << " = $sp + 1 (" << std::dec
       << size << " bytes)" << std::endl;

    uint32_t i = 0, j;
    while (i < size) {
        for (j = 0; j < 8 && i < size; i++, j++)
            os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) m_bus.mem[addr + i] << " ";
        os << std::endl;
    }
}

void processor::Core::print_memory(uint64_t addr, uint32_t bytes) {
    os << "Mem(0x" << std::hex << addr << ":0x" << addr + bytes - 1 << " = { ";
    for (uint32_t i = 0; i < bytes; i++)
        os << "0x" << std::setw(2) << std::setfill('0') << (int) m_bus.mem[addr + i] << " ";
    os << std::dec << "}";
}

bool processor::check_memory(uint64_t addr) { return addr < dram::size; }

bool processor::check_register(uint8_t off) { return off < constants::registers::count; }
