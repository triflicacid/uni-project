#pragma once

#include <string>
#include "location.hpp"

namespace assembler::pre_processor {
    struct Constant {
        Location loc;
        std::string value;
    };
}
