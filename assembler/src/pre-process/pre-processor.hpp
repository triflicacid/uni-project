#pragma once

#include "messages/list.hpp"
#include "data.hpp"

namespace assembler {
  /**
   * @brief Read the source file named in `data.cli_args` into `data.lines`.
   * @param data Pre-processor data to populate.
   * @param msgs Message list to report diagnostics to.
   */
  void read_source_file(pre_processor::Data &data, message::List &msgs);

  /**
   * @brief Read a specific source file into `data.lines` (used for both the main source and includes).
   * @param filepath Path of the file to read.
   * @param data Pre-processor data to populate.
   * @param msgs Message list to report diagnostics to.
   * @param line_no Line number the file was included from (for diagnostics), or -1 for the main source file.
   * @param col_no Column number the file was included from (for diagnostics).
   */
  void
  read_source_file(const std::filesystem::path &filepath, pre_processor::Data &data, message::List &msgs, int line_no,
                   int col_no);

  /**
   * @brief Run pre-processing (macro expansion, constant substitution, includes) over `data`, mutating it in place.
   * @param data Pre-processor data to process.
   * @param msgs Message list to report diagnostics to.
   */
  void pre_process(pre_processor::Data &data, message::List &msgs);

  /**
   * @brief Process a single pre-processor directive within a line.
   * @param data Pre-processor data being built; updated with the directive's effect.
   * @param i Index within `line.second` the directive starts at.
   * @param line_idx Index of `line` within `data.lines`.
   * @param line Line containing the directive.
   * @param current_macro Set/cleared to track a macro definition currently being collected across lines.
   * @param msgs Message list to report diagnostics to.
   */
  void process_directive(pre_processor::Data &data, int i, int line_idx, pre_processor::Line &line,
                         std::pair<std::string, pre_processor::Macro> *&current_macro, message::List &msgs);
}
