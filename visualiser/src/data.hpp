#pragma once

#include "cli_arguments.hpp"
#include "assembly.hpp"

namespace visualiser {
    struct Data {
        CliArguments &cli_args;
        std::unique_ptr<assembly::Data> assembly;

        explicit Data(CliArguments &cli_args) : cli_args(cli_args) {}
    };

    std::unique_ptr<assembly::Data> compile_assembly(const visualiser::Data &data);
}
