#include "chunk.hpp"

#include <iomanip>

namespace assembler {
  void Chunk::print(std::ostream &os) {
    os << "Chunk at +0x" << std::hex << offset << std::dec << " of " << m_size << " bytes";

    if (m_is_data) {
      os << " - data:" << std::endl << '\t' << std::uppercase << std::hex;

      const auto bytes = *get_data();

      for (int i = 0; i < bytes.size(); i++) {
        os << std::setw(2) << std::setfill('0') << (int) bytes[i];

        if (i + 1 < bytes.size()) os << " ";
      }

      std::cout << std::dec << std::endl;
    } else {
      std::cout << " - instruction 0x" << std::hex << get_instruction()->compile() << std::dec << std::endl
                << '\t';
      get_instruction()->debug_print(os);
    }
  }

  void Chunk::write(std::ostream &os) {
    if (m_is_data) {
      for (auto &byte: *get_data()) {
        os.write((char *) &byte, sizeof(byte));
      }
    } else {
      auto data = get_instruction()->compile();
      os.write((char *) &data, sizeof(data));
    }
  }

  void Chunk::set(std::unique_ptr<std::vector<uint8_t>> bytes) {
    m_is_data = true;
    m_size = bytes->size();
    m_bytes = std::move(bytes);
  }

  void Chunk::set(std::unique_ptr<instruction::Instruction> instruction) {
    m_is_data = false;
    m_size = sizeof(uint64_t);
    m_instruction = std::move(instruction);
  }
}
