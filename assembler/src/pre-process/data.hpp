#pragma once

#include <map>

#include "constant.hpp"
#include "macro.hpp"
#include "cli_arguments.hpp"
#include "location.hpp"

namespace assembler::pre_processor {
  typedef std::pair<Location, std::string> Line;

  struct Data {
    CliArguments &cli_args; // CLI options to program
    std::filesystem::path executable; // Path to executable
    std::filesystem::path file_path; // Name of source file (may be different to base.input_filename if parsing an include)
    std::vector<Line> lines; // List of source file lines
    std::map<std::string, Constant> constants; // Map of constant values (%define)
    std::map<std::string, Macro> macros; // Map of macros
    std::map<std::filesystem::path, Location> included_files; // Maps included files to where they were included

    explicit Data(CliArguments &args) : cli_args(args) {}

    Data(CliArguments &args, const Data &data) : Data(args) {
      file_path = data.file_path;
      executable = data.executable;
    }

    /** Set executable path. */
    void set_executable(const std::string &path);

    /** Writes `lines` to buffer. */
    std::ostream &write_lines(std::ostream &os) const;

    /** Merge given data with this. Insert lines starting at `index`. */
    void merge(Data &other, int line_index = -1);
  };
}
