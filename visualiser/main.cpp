#include <cstring>
#include <iostream>
#include <unordered_set>
#include "screen.hpp"
#include "named_fstream.hpp"
#include "sources.hpp"
#include "processor.hpp"
#include "util.hpp"

bool read_file(std::unique_ptr<named_fstream>& file_object, const std::string& option_name, const std::string& filename, std::ios::openmode mode) {
  if (auto stream = named_fstream::open(filename, std::ios::in)) {
    file_object = std::move(stream);
//    visualiser::s_source->canonicalise();
    return true;
  } else {
    std::cout << option_name << ": failed to open file " << filename;
    return false;
  }
}

bool read_file(std::unique_ptr<named_fstream>& file_object, int argc, char **argv, int& i, std::ios::openmode mode) {
  if (++i == argc) {
    std::cout << argv[i - 1] << ": expected file path\n";
    return false;
  }

  return read_file(file_object, argv[i - 1], argv[i], mode);
}

/** Parse command-line arguments. */
int parse_arguments(int argc, char** argv, std::unordered_set<uint64_t>& breakpoints) {
  bool consumed_positional = false;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "--asm")) {
        if (!read_file(visualiser::sources::asm_source, argc, argv, i, std::ios::in)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--bin")) {
        if (!read_file(visualiser::processor::source, argc, argv, i, std::ios::in)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--edel")) {
        if (!read_file(visualiser::sources::edel_source, argc, argv, i, std::ios::in)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--reconstruction")) {
        if (!read_file(visualiser::sources::s_source, argc, argv, i, std::ios::in)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--stdout")) {
        if (!read_file(visualiser::processor::piped_stdout, argc, argv, i, std::ios::out)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "--stdin")) {
        if (!read_file(visualiser::processor::piped_stdin, argc, argv, i, std::ios::in)) {
          return EXIT_FAILURE;
        }
      } else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--breakpoint")) {
        i++;
        breakpoints.clear();
        split_string(argv[i++], ',', [&](const std::string& s) {
          breakpoints.insert(std::stoull(s));
        });
      } else {
        std::cout << "unknown/repeated flag " << argv[i] << std::endl;
        return EXIT_FAILURE;
      }
    } else if (!consumed_positional) {
      consumed_positional = true;
      const std::string base = argv[i++];

      // default source files
      if (!visualiser::sources::asm_source && !read_file(visualiser::sources::asm_source, "--asm", base + ".asm", std::ios::in)) {
        return EXIT_FAILURE;
      }
      if (!visualiser::sources::edel_source && !read_file(visualiser::sources::edel_source, "--edel", base + ".edel", std::ios::in)) {
        return EXIT_FAILURE;
      }
      if (!visualiser::sources::s_source && !read_file(visualiser::sources::s_source, "--reconstructed", base + ".s", std::ios::in)) {
        return EXIT_FAILURE;
      }
      if (!visualiser::processor::source && !read_file(visualiser::processor::source, "--bin", base, std::ios::in)) {
        return EXIT_FAILURE;
      }
    } else {
      std::cout << "unexpected argument '" << argv[i] << "'";
      return EXIT_FAILURE;
    }
  }

  // Check if all files are present
  if (!visualiser::sources::edel_source) {
    std::cout << "expected Edel source file to be provided (.edel extension, --edel option)";
    return EXIT_FAILURE;
  }

  if (!visualiser::sources::s_source) {
    std::cout << "expected reconstructed assembly source file to be provided (.s extension, --reconstructed option)";
    return EXIT_FAILURE;
  }

  if (!visualiser::sources::asm_source) {
    std::cout << "expected assembly source file to be provided (.asm extension, --asm option)";
    return EXIT_FAILURE;
  }

  if (!visualiser::processor::source) {
    std::cout << "expected binary source file to be provided (no extension, --bin option)";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
  std::unordered_set<uint64_t> breakpoints;
  if (int ret = parse_arguments(argc, argv, breakpoints) != EXIT_SUCCESS) {
    return ret;
  }

  // canonicalise file names
  visualiser::sources::edel_source->canonicalise();
  visualiser::sources::asm_source->canonicalise();
  visualiser::sources::s_source->canonicalise();
  visualiser::processor::source->canonicalise();

  // populate assembly data
  visualiser::sources::init();

  // instantiate the processor
  visualiser::processor::init();
  processor::debug::cpu = true;
  processor::debug::args = true;

  for (uint64_t breakpoint : breakpoints) {
    if (auto* pc = visualiser::sources::locate_pc(breakpoint)) {
      pc->set_breakpoint(true);
    } else {
      // warn, but don't exit
      std::cerr << "warning: cannot set breakpoint at $pc=0x" << std::hex << pc << std::dec << " - unable to locate in program" << std::endl;
    }
  }

  // launch application
  visualiser::launch();

  return EXIT_SUCCESS;
}
