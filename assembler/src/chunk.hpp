#pragma once

#include <utility>

#include "instructions/instruction.hpp"

namespace assembler {
  class Chunk {
  public:
    uint32_t offset; // Byte offset

  private:
    Location m_source; // source location
//    std::unique_ptr<instruction::Instruction> m_instruction = nullptr;
//    std::unique_ptr<std::vector<uint8_t>> m_bytes = nullptr;

  public:
    Chunk(Location source, uint32_t offset) : m_source(std::move(source)), offset(offset) {}

    virtual ~Chunk() = default;

    virtual void debug_print(std::ostream &os);

    [[nodiscard]] virtual uint16_t size() const = 0;

    [[nodiscard]] const Location &location() const { return m_source; }

    virtual void write(std::ostream &os) = 0;

    virtual void reconstruct(std::ostream &os) = 0;

    /** Replace given label with its address */
    virtual void replace_label(const std::string& label, uint32_t address, bool debug = false) {}

    /** Return first, if any, label we come across */
    virtual const instruction::ArgumentLabel* get_first_label() const
    { return nullptr; }
  };

  class InstructionChunk : public Chunk {
    std::unique_ptr<instruction::Instruction> m_instruction;

  public:
    InstructionChunk(Location source, uint32_t offset, std::unique_ptr<instruction::Instruction> i)
      : Chunk(std::move(source), offset), m_instruction(std::move(i)) {}

    uint16_t size() const override { return sizeof(uint64_t); }

    void debug_print(std::ostream &os) override;

    void write(std::ostream &os) override;

    void reconstruct(std::ostream &os) override;

    void replace_label(const std::string &label, uint32_t address, bool debug = false) override;

    const instruction::ArgumentLabel* get_first_label() const override;
  };

  class DataChunk : public Chunk {
    std::vector<uint8_t> m_bytes;

  public:
    DataChunk(Location source, uint32_t offset, std::vector<uint8_t> bytes)
      : Chunk(std::move(source), offset), m_bytes(std::move(bytes)) {}

      uint16_t size() const override;

      void debug_print(std::ostream &os) override;

      void write(std::ostream &os) override;

      void reconstruct(std::ostream &os) override;
  };
}
