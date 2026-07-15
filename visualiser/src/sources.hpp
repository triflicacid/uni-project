#pragma once

#include <map>
#include <set>
#include "named_fstream.hpp"
#include "location.hpp"
#include "messages/list.hpp"
#include "graph.hpp"
#include "pair_hash.hpp"

namespace visualiser::sources {
  /** @brief Kind of source file a `File` represents in the source/assembly/language trace chain. */
  enum class Type {
    Source,
    Assembly,
    Language,
  };

  /**
   * @brief One instruction slot in the reconstructed `.s` listing, tracing back to its `.asm` and (if known) `.edel` origin.
   */
  struct PCLine {
    uint64_t pc;
    std::string line; // line in reconstructed source
    int line_no; // line number in reconstructed source
    Location asm_origin; // source in .asm file
    std::optional<Location> lang_origin; // source in .edel file

    /**
     * @brief Check whether a breakpoint is set at this `$pc`.
     * @return True if a breakpoint is set.
     */
    bool has_breakpoint() const;

    /**
     * @brief Set or clear the breakpoint at this `$pc`.
     * @param b True to set the breakpoint, false to clear it.
     */
    void set_breakpoint(bool b) const;

    /** @brief Flip the breakpoint state at this `$pc`. */
    void toggle_breakpoint() const;
  };

  struct FileLine;

  /**
   * @brief A source, assembly, or language file loaded into the visualiser, holding its lines and load state.
   */
  struct File {
    std::filesystem::path path;
    Type type;
    std::vector<FileLine> lines;
    bool loaded = false;

    /**
     * @brief Reconstruct the file's full text by joining its lines.
     * @return The file contents as a single string.
     */
    std::string to_string() const;

    /**
     * @brief Count how many of the file's lines have a breakpoint set.
     * @return The number of breakpoints in the file.
     */
    int count_breakpoints() const;
  };

  /**
   * @brief A single line of a `File`, with a trace back to the `$pc` value(s) it produced (via the `.s` reconstruction).
   */
  struct FileLine {
    File* parent;
    int n;
    std::string line;
    std::vector<PCLine*> pc_trace; // trace back to $pc (.s file)

    /**
     * @brief Check whether this line produced the given `$pc`.
     * @param pc Program counter value to check.
     * @return True if `pc` traces back to this line.
     */
    bool contains_pc(uint64_t pc) const;

    /**
     * @brief Check whether any `$pc` traced from this line has a breakpoint set.
     * @return True if the line has a breakpoint.
     */
    bool has_breakpoint() const;

    /**
     * @brief Get the first `$pc` value produced by this line, if any.
     * @return The line's `$pc`, or empty if this line produced no instructions.
     */
    std::optional<uint64_t> pc() const;
  };

  extern std::unique_ptr<named_fstream> edel_source; // source edel file
  extern std::unique_ptr<named_fstream> asm_source; // source assembly file (output)
  extern std::unique_ptr<named_fstream> s_source; // source assembly file (reconstruction)
  extern std::map<uint32_t, PCLine> pc_to_line; // map byte offset ($pc) to location
  extern std::map<std::filesystem::path, File> files; // map file paths to contents (used for source storing sources)

  /** Graph tracing each file:line to its corresponding line(s) in the other representations (lang <-> asm <-> reconstructed). */
  extern Graph<std::pair<std::filesystem::path, int>, FileLine*, pair_hash> trace;

  /** @brief Load and cross-link the `.s`, `.asm`, and `.edel` sources into `files`, `pc_to_line`, and `trace`. */
  void init();

  /**
   * @brief Look up the `PCLine` for a given `$pc` value.
   * @param pc Program counter value to look up.
   * @return The matching `PCLine`, or null if none is registered.
   */
  const PCLine* locate_pc(uint64_t pc);

  /**
   * @brief Retrieve a registered file, reading its lines from disk on first access.
   * @param path Path of the file to retrieve.
   * @return The loaded file.
   */
  const File* get_file(const std::filesystem::path &path);

  /**
   * @brief Look up the `PCLine` with the given reconstructed-source line number.
   * @param line Line number in the reconstructed `.s` source.
   * @return The matching `PCLine`, or null if none is found.
   */
  const PCLine* locate_line(int line);

  /**
   * @brief Find all `PCLine`s that originate from a given line of a `.asm` file.
   * @param path Path of the `.asm` file.
   * @param line Line number within the file.
   * @return All `PCLine`s tracing back to that line.
   */
  std::vector<PCLine*> locate_asm_line(const std::filesystem::path &path, int line);

  /**
   * @brief Find all `PCLine`s that originate from a given line of a `.edel` (language) file.
   * @param path Path of the language source file.
   * @param line Line number within the file.
   * @return All `PCLine`s tracing back to that line.
   */
  std::vector<PCLine*> locate_lang_line(const std::filesystem::path &path, int line);
}
