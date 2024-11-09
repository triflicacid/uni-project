#pragma once

#include <string>
#include <vector>

namespace assembler::pre_processor {
    struct Macro {
        int line;
        int col;
        std::vector<std::string> params;
        std::vector<std::string> lines; // Lines in macro's body

        Macro(int line, int col, std::vector<std::string> params) : line(line), col(col), params(std::move(params)) {}
    };
}
