#pragma once

#include <map>

#include "constant.hpp"
#include "macro.hpp"
#include "cli_arguments.hpp"
#include "location.hpp"

/** @brief The pre-processing pass: expands `%define`d constants and macros and resolves `%include` directives before the real parser runs. */
namespace assembler::pre_processor {
  /** @brief A single source line paired with the location it came from. */
  typedef std::pair<Location, std::string> Line;

  /** @brief Accumulated state of the pre-processing pass: source lines plus the constants/macros/includes discovered so far. */
  struct Data {
    CliArguments &cli_args; // CLI options to program
    std::filesystem::path executable; // Path to executable
    std::filesystem::path file_path; // Name of source file (may be different to base.input_filename if parsing an include)
    std::vector<Line> lines; // List of source file lines
    std::map<std::string, Constant> constants; // Map of constant values (%define)
    std::map<std::string, Macro> macros; // Map of macros
    std::map<std::filesystem::path, Location> included_files; // Maps included files to where they were included

    /** @brief Construct empty pre-processor data. @param args Parsed command-line arguments. */
    explicit Data(CliArguments &args) : cli_args(args) {}

    /**
     * @brief Construct pre-processor data for an included file, carrying over the parent's file path and executable path.
     * @param args Parsed command-line arguments.
     * @param data Parent data to inherit `file_path`/`executable` from.
     */
    Data(CliArguments &args, const Data &data) : Data(args) {
      file_path = data.file_path;
      executable = data.executable;
    }

    /**
     * @brief Set the path to the assembler executable, used to resolve relative include/library paths.
     * @param path Path to the executable.
     */
    void set_executable(const std::string &path);

    /**
     * @brief Write every line to a stream.
     * @param os Stream to write to.
     * @return `os`, for chaining.
     */
    std::ostream &write_lines(std::ostream &os) const;

    /**
     * @brief Merge another data's lines/constants/macros/includes into this one.
     * @param other Data to merge in.
     * @param line_index Index within `lines` to insert `other`'s lines at, or -1 to append at the end.
     */
    void merge(Data &other, int line_index = -1);
  };
}
