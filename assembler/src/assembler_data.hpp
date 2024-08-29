#pragma once

#include "chunk.hpp"
#include "pre-process/data.hpp"
#include "label.hpp"

namespace assembler {
  struct Data {
    std::filesystem::path file_path; // Name of source file
    bool debug; // Print debug comments?
    std::vector<Line> lines; // List of source file lines
    std::map<std::string, Label> labels;
    uint16_t offset; // byte offset into source
    std::string main_label; // Contain "main" label name
    std::deque<Chunk *> buffer; // List of compiled chunks

    explicit Data(bool debug) {
      this->debug = debug;
      main_label = "main";
      offset = 0;
    }

    explicit Data(const pre_processor::Data &data) : Data(data.debug) {
      file_path = data.file_path;
      lines = data.lines;
    }

    /** Replace all instances of <label> with the given <address>. */
    void replace_label(const std::string &label, uint32_t address) const;

    /** Consider start label, may add instruction to chunks. */
    void add_start_label_jump();

    /** Get size in bytes. */
    [[nodiscard]] uint32_t get_bytes() const;

    /** Write data to output stream. */
    void write(std::ostream &stream) const;
  };
}
