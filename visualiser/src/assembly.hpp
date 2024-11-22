#pragma once

#include <map>
#include <set>
#include "named_fstream.hpp"
#include "location.hpp"
#include "messages/list.hpp"

namespace visualiser::assembly {
    struct PCEntry {
        uint64_t pc;
        std::string line; // line in reconstructed source
        int line_no; // line number in reconstructed source
        Location origin; // source location
    };

    extern std::unique_ptr<named_fstream> source; // source assembly file (reconstruction)
    extern std::map<uint32_t, PCEntry> pc_to_line; // map byte offset ($pc) to location
    extern std::map<std::filesystem::path, std::vector<std::string>> files; // map file paths to contents (used for source storing sources)

    /** Initialise internal data from `source` */
    void init();

    // get location in sources from $pc value (may be NULL)
    PCEntry *locate_pc(uint64_t pc);

    // get lines in a file
    const std::vector<std::string> &get_file_lines(const std::filesystem::path &path);

    // given line number is reconstructed source, return $pc entry
    const PCEntry *locate_line(int line);

    // given a Location in assembly, return a list of PCEntries
    std::vector<const PCEntry *> locate_asm_line(const std::filesystem::path &path, int line);
}
