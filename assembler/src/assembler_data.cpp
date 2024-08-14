#include "assembler_data.hpp"

namespace assembler {
  void Data::replace_label(const std::string &label, uint32_t address) {
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

  int Data::get_bytes() {
    if (buffer.empty())
      return 0;

    auto last = buffer.back();
    return last->get_offset() + last->get_bytes();
  }

  void Data::write_headers(std::ostream &stream) {
    if (buffer.empty())
      return;

    // Write start address
    auto start_label = labels.find(main_label);
    uint64_t start_addr = start_label == labels.end()
                          ? (section_text == -1 ? 0 : section_text)
                          : start_label->second.addr;
    stream.write((char *) &start_addr, sizeof(start_addr));
  }

  void Data::write_chunks(std::ostream &stream) {
    for (auto chunk: buffer) {
      chunk->write(stream);
    }
  }
}
