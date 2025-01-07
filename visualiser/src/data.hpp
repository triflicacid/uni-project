#pragma once

#include <map>
#include <set>
#include "named_fstream.hpp"
#include "location.hpp"
#include "messages/list.hpp"

namespace visualiser {
  enum class Type {
    Source,
    Assembly,
    Language,
  };

  struct PCLine {
    uint64_t pc;
    std::string line; // line in reconstructed source
    int line_no; // line number in reconstructed source
    Location asm_origin, lang_origin; // source locations
  };

  struct FileLine {
    int n;
    std::string line;
    std::vector<const PCLine*> trace;
  };

  struct File {
    std::filesystem::path path;
    Type type;
    std::vector<FileLine> lines;
    bool loaded;
  };

  extern std::unique_ptr<named_fstream> source; // source assembly file (reconstruction)
  extern std::map<uint32_t, PCLine> pc_to_line; // map byte offset ($pc) to location
  extern std::map<std::filesystem::path, File> files; // map file paths to contents (used for source storing sources)

  /** Initialise internal data from `source` */
  void init();

  // get location in sources from $pc value (may be NULL)
  PCLine *locate_pc(uint64_t pc);

  // retrieve a file, load its lines if needed
  const File* get_file(const std::filesystem::path &path);

  // get the PCLine with the given line number (in the source)
  const PCLine* locate_line(int line);

  // get the PCLines in the given .asm file and line
  std::vector<const PCLine*> locate_asm_line(const std::filesystem::path &path, int line);
}
