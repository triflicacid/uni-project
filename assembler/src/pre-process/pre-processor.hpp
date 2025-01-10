#pragma once

#include "messages/list.hpp"
#include "data.hpp"

namespace assembler {
  /** Read source file provided in cli_args. */
  void read_source_file(pre_processor::Data &data, message::List &msgs);

  /** Read source file provided. */
  void
  read_source_file(const std::filesystem::path &filepath, pre_processor::Data &data, message::List &msgs, int line_no,
                   int col_no);

  /** Run pre-processing on the given data, mutating it, or add error. */
  void pre_process(pre_processor::Data &data, message::List &msgs);

  /** Process a directive, starting from `index`. */
  void process_directive(pre_processor::Data &data, int i, int line_idx, pre_processor::Line &line,
                         std::pair<std::string, pre_processor::Macro> *&current_macro, message::List &msgs);
}
