#include <cstring>
#include <iostream>
#include "data.hpp"

/** Parse command-line arguments. */
int parse_arguments(int argc, char **argv, visualiser::CliArguments &opts) {
    std::string out_dir_suffix = ".visualiser", asm_path_prefix = "assembler";

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'd' && !opts.debug) {
                // Enable debug mode
                opts.debug = true;
            } else if (!strcmp(argv[i], "--out-dir")) {
                out_dir_suffix = ++i < argc ? argv[i] : "";
            } else if (!strcmp(argv[i], "--asm-lib")) {
                opts.assembler_lib = ++i < argc ? argv[i] : "";
            } else if (!strcmp(argv[i], "--assembler")) {
                // Provide path to assembler
                if (++i == argc) {
                    std::cout << argv[i] << ": expected file path";
                    return EXIT_FAILURE;
                }

                asm_path_prefix = argv[i];
            } else {
                std::cout << "Unknown/repeated flag " << argv[i] << std::endl;
                return EXIT_FAILURE;
            }
        } else if (!opts.source) {
            if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
                opts.source = std::move(stream);
            } else {
                std::cout << "positional #" << i << ": failed to open file " << argv[i];
                return EXIT_FAILURE;
            }
        } else {
            std::cout << "Unexpected argument '" << argv[i] << "'";
            return EXIT_FAILURE;
        }
    }

    // Check if all files are present
    if (!opts.source) {
        std::cout << "Expected source file to be provided";
        return EXIT_FAILURE;
    }

    if (opts.debug)
        std::cout << "source file: " << opts.source->path << std::endl;

    // check that lib directory exists
    opts.out_dir = weakly_canonical(canonical(opts.source->path).parent_path() / out_dir_suffix);

    if (!exists(opts.out_dir)) {
        // that's fine, we can create the child... but only if the parent exists
        if (!exists(opts.out_dir.parent_path())) {
            std::cout << "output directory " << opts.out_dir << " cannot be created: parent " << opts.out_dir.parent_path() << " does not exist";
            return EXIT_FAILURE;
        }

        create_directory(opts.out_dir);
    }

    if (opts.debug) std::cout << "output directory: " << opts.out_dir << std::endl;
    if (!is_directory(opts.out_dir)) {
        std::cout << "output directory " << opts.out_dir << " does not point to a directory";
        return EXIT_FAILURE;
    }

    // check that assembler exists
    opts.assembler_path = weakly_canonical(std::filesystem::current_path() / asm_path_prefix);

    if (!exists(opts.assembler_path)) {
        std::cout << "assembler " << opts.assembler_path << " does not exist" ;
        return EXIT_FAILURE;
    }

    if (opts.debug) std::cout << "assembler: " << opts.assembler_path << std::endl;

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    using namespace visualiser;

    CliArguments args;

    if (auto ret = parse_arguments(argc, argv, args) != EXIT_SUCCESS) {
        return ret;
    }

    // create data
    Data data(args);
    message::List messages;

    // handle the file extension
    const auto ext = args.source->path.extension();

    if (auto it = assembly::file_extensions.find(ext) != assembly::file_extensions.end()) {
        if (!(data.assembly = compile_assembly(data))) {
            return EXIT_FAILURE;
        }

        data.assembly->populate();
    } else {
        // unknown file extension?
        std::cout << "unrecognised file extension " << ext;
    }

    return EXIT_SUCCESS;
}
