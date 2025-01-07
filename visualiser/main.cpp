#include <cstring>
#include <iostream>
#include "screen.hpp"
#include "named_fstream.hpp"
#include "data.hpp"
#include "processor.hpp"

/** Parse command-line arguments. */
int parse_arguments(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "--asm")) {
        if (++i == argc) {
          std::cout << argv[i - 1] << ": expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
          visualiser::source = std::move(stream);
          visualiser::source->canonicalise();
        } else {
          std::cout << argv[i - 1] << ": failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--bin")) {
        if (++i == argc) {
          std::cout << argv[i - 1] << ": expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
          visualiser::processor::source = std::move(stream);
        } else {
          std::cout << argv[i - 1] << ": failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--stdout")) {
        if (++i == argc) {
          std::cout << argv[i - 1] << ": expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
          visualiser::processor::piped_stdout = std::move(stream);
        } else {
          std::cout << argv[i - 1] << ": failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--stdin")) {
        if (++i == argc) {
          std::cout << argv[i - 1] << ": expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
          visualiser::processor::piped_stdin = std::move(stream);
        } else {
          std::cout << argv[i - 1] << ": failed to open file " << argv[i];
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
  if (!visualiser::source) {
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
  visualiser::init();

  // instantiate the processor
  visualiser::processor::init();
  processor::debug::cpu = true;

  // launch application
  visualiser::launch();

  return EXIT_SUCCESS;
}
