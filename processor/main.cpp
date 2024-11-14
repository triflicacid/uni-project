#include "cpu.hpp"
#include "debug.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "cli_arguments.hpp"

int parse_arguments(int argc, char **argv, processor::CliArguments &args) {
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        if (arg[0] == '-') {
            if (arg == "-o") {
                if (++i >= argc) {
                    std::cerr << arg << ": expected file path.";
                    return EXIT_FAILURE;
                }

                if (!(args.output_file = named_fstream::open(argv[i], std::ios::out))) {
                    std::cerr << arg << ": failed to open file '" << argv[i] << "'";
                    return EXIT_FAILURE;
                }
            } else if (arg == "-i") {
                if (++i >= argc) {
                    std::cerr << arg << ": expected file path.";
                    return EXIT_FAILURE;
                }

                if (!(args.input_file = named_fstream::open(argv[i], std::ios::in))) {
                    std::cerr << arg << ": failed to open file '" << argv[i] << "'";
                    return EXIT_FAILURE;
                }
            } else {
                std::cerr << "unknown flag " << arg;
                return EXIT_FAILURE;
            }

            continue;
        }

        if (!args.source_file) {
            if (!(args.source_file = named_fstream::open(arg, std::ios::in | std::ios::binary))) {
                std::cerr << "source file: failed to open file '" << arg << "'";
                return EXIT_FAILURE;
            }

            continue;
        }

        std::cerr << "unknown argument " << arg;
        return EXIT_FAILURE;
    }

    // check that we have a source file
    if (!args.source_file) {
        std::cerr << "no source file provided";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    using namespace processor;

    // parse command line arguments
    CliArguments args;

    if (parse_arguments(argc, argv, args) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    args.debug.cpu = true;
    args.debug.errs = true;
    args.debug.flags = true;

    // create and initialise CPU
    CPU cpu(args.output_file ? args.output_file->stream : std::cout, args.input_file ? args.input_file->stream : std::cin, args.debug);

    // determine file size
    auto &stream = args.source_file->stream;
    stream.seekg(-1, std::ios::end);
    size_t file_size = stream.tellg();
    stream.seekg(std::ios::beg);

    if (args.debug.cpu) std::cout << DEBUG_STR " reading source file " << args.source_file->path << "... " << file_size << " bytes read" << std::endl;

    // error if file size exceeds buffer size
    if (file_size >= dram::size) {
        std::cerr << ERROR_STR " source file size of " << file_size << " bytes exceeds memory size of " << dram::size << std::endl;
        return EXIT_FAILURE;
    }

    // extract words in header
    uint64_t addr_entry, addr_interrupt;
    stream.read((char *) &addr_entry, sizeof(addr_entry));
    stream.read((char *) &addr_interrupt, sizeof(addr_interrupt));

    if (args.debug.cpu) std::cout << DEBUG_STR " entry point: 0x" << std::hex << addr_entry << "; interrupt handler: 0x" << addr_interrupt << std::dec << std::endl;
    cpu.jump(addr_entry);
    cpu.set_interrupt_handler(addr_interrupt);

    // copy code into processor's memory
    stream.read((char *) cpu.get_bus().mem.data(), file_size);

    // start processor
    cpu.step_cycle();

    cpu.print_error(true);

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR " process exited with code 0x%x\n", cpu_exit_code(&cpu));
#endif
    auto err_code = cpu.get_error();
    uint64_t code = err_code ? err_code : cpu.get_return_value();
    if (args.debug.cpu) std::cout << DEBUG_STR " processor exited with code " << code << std::endl;

    return EXIT_SUCCESS;
}
