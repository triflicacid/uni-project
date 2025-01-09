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

    // test if this $pc has a breakpoint
    bool has_breakpoint() const;

    // add/remove breakpoint
    void set_breakpoint(bool) const;

    // toggle breakpoint for this $pc
    void toggle_breakpoint() const;
  };

  struct FileLine;

  struct File {
    std::filesystem::path path;
    Type type;
    std::vector<FileLine> lines;
    bool loaded;

    std::string to_string() const;

    // return the number of breakpoints in thes file
    int count_breakpoints() const;
  };

  struct FileLine {
    int n;
    std::string line;
    std::vector<const PCLine*> trace;

    // test if this line produced (traces to) the given pc
    bool contains_pc(uint64_t pc) const;

    // test if there is a breakpoint in this line
    bool has_breakpoint() const;

    // return $pc of this line (if possible)
    std::optional<uint64_t> pc() const;
  };

  extern std::unique_ptr<named_fstream> source; // source assembly file (reconstruction)
  extern std::map<uint32_t, PCLine> pc_to_line; // map byte offset ($pc) to location
  extern std::map<std::filesystem::path, File> files; // map file paths to contents (used for source storing sources)

  /** Initialise internal data from `source` */
  void init();

  // get location in sources from $pc value (may be NULL)
  const PCLine* locate_pc(uint64_t pc);

  // retrieve a file, load its lines if needed
  const File* get_file(const std::filesystem::path &path);

  // get the PCLine with the given line number (in the source)
  const PCLine* locate_line(int line);

  // find all PCLines which originate from path.asm:line
  std::vector<const PCLine*> locate_asm_line(const std::filesystem::path &path, int line);

  // find all PCLines which originate from path.src:line
  std::vector<const PCLine*> locate_lang_line(const std::filesystem::path &path, int line);
}
