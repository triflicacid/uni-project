#pragma once

#include "named_fstream.hpp"

namespace processor {
    struct CliArguments {
        std::unique_ptr<named_fstream> source_file;
        std::unique_ptr<named_fstream> input_file;
        std::unique_ptr<named_fstream> output_file;
        Debug debug;
    };
}
