#pragma once

#include "named_fstream.hpp"

namespace processor {
  /** @brief Parsed command-line file arguments for the processor executable. */
  struct CliArguments {
    /** @brief Binary file containing the program to execute. */
    std::unique_ptr<named_fstream> source_file;
    /** @brief File the program reads its input from, if given (otherwise stdin is used). */
    std::unique_ptr<named_fstream> input_file;
    /** @brief File the program writes its output to, if given (otherwise stdout is used). */
    std::unique_ptr<named_fstream> output_file;
    /** @brief File debug trace output is written to, if given. */
    std::unique_ptr<named_fstream> debug_file;
  };
}
