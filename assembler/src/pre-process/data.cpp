#include "data.hpp"

namespace assembler::pre_processor {
  void Data::set_executable(const std::string &path) {
    executable = canonical(std::filesystem::path(path));
  }

  std::ostream &Data::write_lines(std::ostream &os) const {
    for (const auto &line: lines) {
      os << line.second << std::endl;
    }

    return os;
  }

  void Data::merge(Data &other, int line_index) {
    // Merge lines
    auto lines_ptr = line_index < 0 ? lines.end() - 1 + line_index : lines.begin() + line_index;
    lines.insert(lines_ptr, other.lines.begin(), other.lines.end());

    // Merge constants
    constants.insert(other.constants.begin(), other.constants.end());

    // Merge macros
    macros.insert(other.macros.begin(), other.macros.end());
  }
}
