#pragma once

#include <map>

#include "Source.hpp"
#include "statement/Function.hpp"

namespace language {
    struct CompiledProgram {
        const Source *source;
        std::map<int, const statement::Function *> declared_funcs; // Functions which were declared only
        std::map<int, std::pair<const statement::Function *, std::stringstream>> defined_funcs; // Functions which were defined, along with assembly body

        void debug_print(std::ostream& stream, const std::string& prefix = "");
    };
}
