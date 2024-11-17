#include <cstring>
#include <iostream>
#include "screen.hpp"
#include "named_fstream.hpp"
#include "assembly.hpp"
#include "processor.hpp"

/** Parse command-line arguments. */
int parse_arguments(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "--asm")) {
                if (++i == argc) {
                    std::cout << "--asm: expected file path\n";
                    return EXIT_FAILURE;
                }

                if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
                    visualiser::assembly::source = std::move(stream);
                } else {
                    std::cout << "--asm: failed to open file " << argv[i];
                    return EXIT_FAILURE;
                }
            } else if (!strcmp(argv[i], "--bin")) {
                if (++i == argc) {
                    std::cout << "--bin: expected file path\n";
                    return EXIT_FAILURE;
                }

                if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
                    visualiser::processor::source = std::move(stream);
                } else {
                    std::cout << "--bin: failed to open file " << argv[i];
                    return EXIT_FAILURE;
                }
            } else {
                std::cout << "Unknown/repeated flag " << argv[i] << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            std::cout << "Unexpected argument '" << argv[i] << "'";
            return EXIT_FAILURE;
        }
    }

    // Check if all files are present
    if (!visualiser::assembly::source) {
        std::cout << "Expected assembly source file to be provided";
        return EXIT_FAILURE;
    }

    if (!visualiser::processor::source) {
        std::cout << "Expected binary source file to be provided";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (auto ret = parse_arguments(argc, argv) != EXIT_SUCCESS) {
        return ret;
    }

    // populate assembly data
    visualiser::assembly::populate();

    // instantiate the processor
    processor::debug::cpu = true;
    processor::read_binary_file(visualiser::processor::cpu, visualiser::processor::source->stream);
    visualiser::processor::cpu.ds = &visualiser::processor::debug_stream;

    // launch application
    visualiser::init();
    visualiser::start();

    return EXIT_SUCCESS;
}
