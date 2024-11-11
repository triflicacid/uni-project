#pragma once

#include <utility>

#include "instructions/instruction.hpp"

namespace assembler {
    class Chunk {
    public:
        uint32_t offset; // Byte offset

    private:
        bool m_is_data; // Is data or an instruction?
        uint16_t m_size;
        Location m_source; // source location
        std::unique_ptr<instruction::Instruction> m_instruction = nullptr;
        std::unique_ptr<std::vector<uint8_t>> m_bytes = nullptr;

    public:
        Chunk(Location source, uint32_t offset) : m_source(std::move(source)), offset(offset), m_is_data(true), m_size(0) {}

        void print(std::ostream &os);

        /** Are we representing data? */
        [[nodiscard]] bool is_data() const { return m_is_data; }

        [[nodiscard]] uint16_t size() const { return m_size; }

        [[nodiscard]] const Location &location() const { return m_source; }

        [[nodiscard]] std::unique_ptr<instruction::Instruction> &get_instruction() { return m_instruction; }

        [[nodiscard]] std::unique_ptr<std::vector<uint8_t>> &get_data() { return m_bytes; }

        void set(std::unique_ptr<std::vector<uint8_t>> bytes);

        void set(std::unique_ptr<instruction::Instruction> instruction);

        void write(std::ostream &os);
    };
}
