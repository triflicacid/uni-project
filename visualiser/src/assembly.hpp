#pragma once

#include <map>
#include <set>
#include "named_fstream.hpp"
#include "location.hpp"
#include "messages/list.hpp"

namespace visualiser::assembly {
    struct Data {
        const named_fstream &source; // source assembly file
        std::unique_ptr<named_fstream> reconstruction; // '-r' assembly reconstruction
        std::map<uint32_t, std::pair<Location, std::string>> lines; // map byte offset ($ip) to source location

        explicit Data(const named_fstream &source) : source(source) {}

        /** Populate this->lines etc. from reconstructed file. */
        void populate();
    };

    /** Set of valid assembly file extensions. */
    extern std::set<std::string> file_extensions;
}
