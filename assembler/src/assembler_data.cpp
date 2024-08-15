#include "assembler_data.hpp"

namespace assembler {
  void Data::replace_label(const std::string &label, uint32_t address) const {
    for (const auto &chunk: buffer) {
      if (!chunk->is_data()) {
        for (auto &arg: chunk->get_instruction()->args) {
          if (arg.is_label() && *arg.get_label() == label) {
            arg.update(instruction::ArgumentType::Immediate, address);
          }
        }
      }
    }
  }

  uint32_t Data::get_bytes() const {
    if (buffer.empty())
      return 0;

    const auto last = buffer.back();
    return last->get_offset() + last->get_bytes();
  }

  void Data::write_headers(std::ostream &stream) const {
    if (buffer.empty())
      return;

    // Write start address
    const auto start_label = labels.find(main_label);
    uint64_t start_addr = start_label == labels.end()
                            ? 0
                            : start_label->second.addr;
    stream.write((const char *) &start_addr, sizeof(start_addr));
  }

  void Data::write_chunks(std::ostream &stream) const {
    uint32_t offset = 0;

    for (const auto chunk: buffer) {
      while (chunk->get_offset() > offset) {
        stream << 0x00;
        offset++;
      }

      chunk->write(stream);
      offset += chunk->get_bytes();
    }
  }
}
