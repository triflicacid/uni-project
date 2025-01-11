#include <iostream>
#include <memory>
#include <vector>
#include "named_fstream.hpp"
#include "config.hpp"
#include "lexer.hpp"
#include "shell.hpp"

struct Options {
  std::vector<std::unique_ptr<named_fstream>> files; // input files
};

int parse_arguments(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'd') { // debug
        lang::conf::debug = true;
      } else {
        std::cerr << "Unknown flag " << argv[i] << std::endl;
        return EXIT_FAILURE;
      }
    } else if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
      // TODO check for repeated file name
      options.files.push_back(std::move(stream));
    } else {
      std::cerr << "positional: failed to open file " << argv[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
  Options options;
  if (int code = parse_arguments(argc, argv, options); code != EXIT_SUCCESS) {
    return code;
  }

  // for each source...
  for (std::unique_ptr<named_fstream>& file : options.files) {
    IStreamWrapper wrapper(std::move(*file->take()));
    lang::lexer::Lexer lexer(wrapper);

    while (true) {
      auto token = lexer.next();
      std::cout << "Token@<" << token.source.line() + 1 << ":" << token.source.column() + 1 << "> " << ANSI_GREEN << token.image << ANSI_RESET << std::endl;
      if (token.is_eof()) break;
    }
  }

  return EXIT_SUCCESS;
}
