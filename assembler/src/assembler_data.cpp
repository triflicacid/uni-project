#include "assembler_data.hpp"

#include <processor/src/constants.h>

namespace assembler {
  void Data::replace_label(const std::string &label, uint32_t address) const {
    for (const auto &chunk: buffer) {
      if (!chunk->is_data()) {
        const auto &inst = chunk->get_instruction();

        for (uint8_t i = 0; i < inst->args.size(); i++) {
          auto &arg = inst->args[i];

          if (arg.is_label() && *arg.get_label() == label) {
            std::cout << "Replace label " << label << " with address 0x" << std::hex << address << std::dec << std::endl;
            arg.update(inst->signature->arguments[inst->overload][i] == instruction::ArgumentType::Address
                         ? instruction::ArgumentType::Address
                         : instruction::ArgumentType::Immediate, address);
          }
        }
      }
    }
  }

  void Data::add_start_label_jump() {
    const auto start_label = labels.find(main_label);
    if (start_label == labels.end()) return;

    // create jump instruction
    std::string opts;
    auto jump = new instruction::Instruction(&instruction::Signature::_load, {
                                               instruction::Argument(instruction::ArgumentType::Register, REG_IP),
                                               instruction::Argument(instruction::ArgumentType::Immediate,
                                                                     start_label->second.addr + 8),
                                             });

    // increase all existing chunks' offsets and offset address references
    for (const auto &chunk: buffer) {
      chunk->offset += 8;

      if (!chunk->is_data()) {
        chunk->get_instruction()->offset_addresses(1);
      }
    }

    // add jmp instruction to buffer
    const auto chunk = new Chunk(-1, 0);
    chunk->set_instruction(jump);
    buffer.push_front(chunk);
  }

  uint32_t Data::get_bytes() const {
    if (buffer.empty())
      return 0;

    const auto last = buffer.back();
    return last->offset + last->get_bytes();
  }

  void Data::write(std::ostream &stream) const {
    uint32_t offset = 0; // current position in stream

    // write chunks, filling in gaps between chunks as required for contiguous layout
    for (const auto chunk: buffer) {
      while (chunk->offset > offset) {
        stream << 0x00;
        offset++;
      }

      chunk->write(stream);
      offset += chunk->get_bytes();
    }
  }
}
