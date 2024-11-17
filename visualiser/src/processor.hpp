#pragma once

#include "../processor/src/cpu.hpp"
#include "named_fstream.hpp"

namespace visualiser::processor {
    extern ::processor::CPU cpu;
    extern std::unique_ptr<named_fstream> source;
    extern std::ostringstream debug_stream;
}
