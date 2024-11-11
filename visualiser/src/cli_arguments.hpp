#pragma once

#include "util/named_fstream.hpp"
#include <memory>

namespace visualiser {
    struct CliArguments {
        std::unique_ptr<named_fstream> source;
        std::filesystem::path assembler_path; // path to the assembler executable
        std::optional<std::string> assembler_lib; // assembler's -l flag
        std::filesystem::path out_dir; // directory for output files
        bool debug;
    };
}
