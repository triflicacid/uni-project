#include "assembler_data.hpp"
#include "instructions/signature.hpp"

#include "processor/src/constants.hpp"

namespace assembler {
    void Data::replace_label(const std::string &label, uint32_t address) const {
        for (const auto &chunk: buffer) {
            if (!chunk->is_data()) {
                const auto &inst = chunk->get_instruction();

                for (uint8_t i = 0; i < inst->args.size(); i++) {
                    auto &arg = inst->args[i];

                    if (arg.is_label() && *arg.get_label() == label) {
                        if (cli_args.debug)
                            std::cout << "Replace label " << label << " with address 0x" << std::hex << address
                                      << std::dec << std::endl;
                        arg.update(inst->signature->arguments[inst->overload][i] == instruction::ArgumentType::Address
                                   ? instruction::ArgumentType::Address
                                   : instruction::ArgumentType::Immediate, address);
                    }
                }
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
        value = label == labels.end() ? processor::constants::default_interrupt_handler : label->second.addr;
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
