#include "chunk.hpp"

#include <iomanip>

namespace assembler {
  void Chunk::debug_print(std::ostream &os) {
    os << "Chunk at +0x" << std::hex << offset << std::dec << " of " << size() << " bytes";
  }

  void InstructionChunk::debug_print(std::ostream& os) {
    Chunk::debug_print(os);

    std::cout << " - instruction 0x" << std::hex << m_instruction->compile() << std::dec << std::endl
              << '\t';
    m_instruction->debug_print(os);
  }

  void InstructionChunk::write(std::ostream& os) {
    uint64_t data = m_instruction->compile();
    os.write((char *) &data, sizeof(data));
  }

  void InstructionChunk::reconstruct(std::ostream& os) {
    m_instruction->print(os);
  }

  void InstructionChunk::replace_label(const std::string& label, uint32_t address, bool debug) {
    m_instruction->replace_label(label, address, debug);
  }

  const instruction::ArgumentLabel* InstructionChunk::get_first_label() const {
    for (auto &arg: m_instruction->args) {
      if (arg.is_label()) {
        return arg.get_label();
      }
    }

    return nullptr;
  }

  uint16_t DataChunk::size() const {
    return m_bytes.size();
  }

  void DataChunk::debug_print(std::ostream& os) {
    Chunk::debug_print(os);
    os << " - data:" << std::endl << '\t' << std::uppercase << std::hex;

    for (int i = 0; i < m_bytes.size(); i++) {
      os << std::setw(2) << std::setfill('0') << int(m_bytes[i]);
      if (i + 1 < m_bytes.size()) os << " ";
    }

    std::cout << std::dec << std::endl;
  }

  void DataChunk::write(std::ostream& os) {
    for (uint8_t byte: m_bytes) {
      os.write((char *) &byte, sizeof(byte));
    }
  }

  void DataChunk::reconstruct(std::ostream& os) {
    os << ".byte" << std::hex;

    for (uint8_t byte: m_bytes) {
      os << " 0x" << int(byte);
    }

    os << std::dec;
  }

  uint16_t SpaceDirectiveChunk::size() const {
    return m_value;
  }

  void SpaceDirectiveChunk::debug_print(std::ostream& os) {
    Chunk::debug_print(os);
    os << " - space (0x" << std::hex << m_value << std::dec << ")" << std::endl;
  }

  void SpaceDirectiveChunk::write(std::ostream& os) {
    for (uint32_t i = 0; i < m_value; i++) {
      os << 0x00;
    }
  }

  void SpaceDirectiveChunk::reconstruct(std::ostream& os) {
    os << ".space 0x" << std::hex << m_value << std::dec;
  }
}
