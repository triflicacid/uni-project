#pragma once

#include "chunk.hpp"
#include "pre-process/data.hpp"
#include "label.hpp"

namespace assembler {
    struct Data {
        CliArguments &cli_args;
        std::filesystem::path file_path; // Name of source file
        std::vector<Line> lines; // List of source file lines
        std::map<std::string, Label> labels;
        uint16_t offset; // byte offset into source
        std::string main_label; // Contain "main" label name
        std::string interrupt_label; // Contains "interrupt_handler" label name
        std::deque<std::unique_ptr<Chunk>> buffer; // List of compiled chunks

        explicit Data(CliArguments &cli_args) : cli_args(cli_args), offset(0) {
            main_label = "main";
            interrupt_label = "interrupt_handler";
        }

        /** Add a new chunk. */
        void add_chunk(std::unique_ptr<Chunk> chunk);

        /** Replace all instances of <label> with the given <address>. */
        void replace_label(const std::string &label, uint32_t address) const;

        /** Get size in bytes. */
        [[nodiscard]] uint32_t get_bytes() const;

        /** Write data to output stream. */
        void write(std::ostream &stream) const;
    };
}
