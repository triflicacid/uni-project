#pragma once

#include "named_fstream.hpp"
#include <memory>

namespace assembler {
  /** @brief Parsed command-line arguments for the assembler executable. */
  struct CliArguments {
    /** @brief Source assembly file to assemble. */
    std::unique_ptr<named_fstream> source;
    /** @brief File the assembled machine code is written to. */
    std::unique_ptr<named_fstream> output_file;
    /** @brief File the pre-processed (macro-expanded) assembly is written to, if requested. */
    std::unique_ptr<named_fstream> post_processing_file;
    /** @brief Path to the directory containing library files to search for includes. */
    std::filesystem::path lib_path;
    /** @brief Whether debug output is enabled. */
    bool debug = false;
    /** @brief Whether to run the assembly (compilation) stage. */
    bool do_compilation = true;
    /** @brief Whether to run the pre-processing (macro-expansion) stage. */
    bool do_pre_processing = true;
    /** @brief File the reconstructed assembly (assembled then printed back as text) is written to, if requested. */
    std::unique_ptr<named_fstream> reconstructed_asm_file;
  };
}
