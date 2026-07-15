#pragma once

#include "chunk.hpp"
#include "pre-process/data.hpp"
#include "label.hpp"

/** @brief The assembler: turns pre-processed Edel assembly source into an encoded binary Chunk, via parsing, instruction resolution, and label linking. */
namespace assembler {
  /** @brief Accumulated state of the assembly pass: source lines, discovered labels, and the resulting compiled chunks. */
  struct Data {
    CliArguments &cli_args; ///< Parsed command-line arguments.
    std::filesystem::path file_path; ///< Path of the source file being assembled.
    std::vector<pre_processor::Line> lines; ///< Pre-processed source file lines.
    std::map<std::string, Label> labels; ///< Discovered labels, keyed by name.
    uint16_t offset; ///< Current byte offset into the compiled output.
    std::string main_label; ///< Name of the label marking the program's entry point (default "main").
    std::string interrupt_label; ///< Name of the label marking the interrupt handler (default "interrupt_handler").
    std::deque<std::unique_ptr<Chunk>> buffer; ///< Compiled chunks, in emission order.

    /** @brief Construct empty assembly data. @param cli_args Parsed command-line arguments. */
    explicit Data(CliArguments &cli_args) : cli_args(cli_args), offset(0) {
      main_label = "main";
      interrupt_label = "interrupt_handler";
    }

    /** @brief Construct assembly data from the pre-processor's output, taking over its lines. @param data Pre-processor output to continue from. */
    explicit Data(pre_processor::Data &data) : Data(data.cli_args) {
      lines = data.lines;
    }

    /**
     * @brief Append a compiled chunk to the buffer.
     * @param chunk Chunk to add; ownership is transferred.
     */
    void add_chunk(std::unique_ptr<Chunk> chunk);

    /**
     * @brief Replace every reference to a label, across all chunks, with its resolved address.
     * @param label Label name to replace.
     * @param address Address to replace it with.
     */
    void replace_label(const std::string &label, uint32_t address) const;

    /** @brief Get the total size in bytes of every chunk in the buffer. @return Total byte size. */
    [[nodiscard]] uint32_t get_bytes() const;

    /**
     * @brief Write every chunk's compiled bytes to a stream, in order.
     * @param stream Stream to write to.
     */
    void write(std::ostream &stream) const;
  };
}
