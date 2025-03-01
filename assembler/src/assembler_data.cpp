#include "assembler_data.hpp"
#include "instructions/signature.hpp"

#include "constants.hpp"

namespace assembler {
  void Data::replace_label(const std::string &label, uint32_t address) const {
    for (const auto &chunk: buffer) {
      if (!chunk->is_data()) {
        const auto &inst = chunk->get_instruction();
        chunk->get_instruction()->replace_label(label, address, cli_args.debug);
      }
    }
  }

  uint32_t Data::get_bytes() const {
    if (buffer.empty())
      return 0;

    const auto &last = buffer.back();
    return last->offset + last->size();
  }

  void Data::write(std::ostream &stream) const {
    uint32_t offset = 0; // current position in ram

    // write header
    // entry point
    auto label = labels.find(main_label);
    uint64_t value = label == labels.end() ? 0 : label->second.addr;
    stream.write((char *) &value, sizeof(value));

    if (cli_args.debug)
      std::cout << "start address: 0x" << std::hex << value << std::dec << std::endl;

    // interrupt handler
    label = labels.find(interrupt_label);
    value = label == labels.end() ? constants::default_interrupt_handler : label->second.addr;
    stream.write((char *) &value, sizeof(value));

    if (cli_args.debug)
      std::cout << "interrupt handler address: 0x" << std::hex << value << std::dec << std::endl;

    // write chunks, filling in gaps between chunks as required for contiguous layout
    for (auto &chunk: buffer) {
      while (chunk->offset > offset) {
        stream << 0x00;
        offset++;
      }

      chunk->write(stream);
      offset += chunk->size();
    }
  }

  void Data::add_chunk(std::unique_ptr<Chunk> chunk) {
    offset += chunk->size();
    buffer.push_back(std::move(chunk));
  }
}
