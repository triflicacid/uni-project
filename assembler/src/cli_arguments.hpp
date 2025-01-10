#pragma once

#include "named_fstream.hpp"
#include <memory>

namespace assembler {
  struct CliArguments {
    std::unique_ptr<named_fstream> source; // source file
    std::unique_ptr<named_fstream> output_file; // file for compiler machine code
    std::unique_ptr<named_fstream> post_processing_file; // file for post-processed assembly
    std::filesystem::path lib_path; // path to lib folder
    bool debug;
    bool do_compilation = true;
    bool do_pre_processing = true;
    std::unique_ptr<named_fstream> reconstructed_asm_file; // file for reconstructed assembly
  };
}
