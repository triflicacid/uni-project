#include <iostream>
#include <fstream>
#include <cstring>
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"

struct Options {
  std::vector<char *> input_files{};
  char *output_dir;
  bool output_lexed;
  char *output_file;
  bool output_parsed;
  bool output_modules;
  bool debug;

  Options() {
    output_file = nullptr;
    output_lexed = false;
    output_parsed = false;
    output_dir = nullptr;
    output_modules = false;
    debug = false;
  }
};

/** Parse command-line arguments. */
int parse_arguments(int argc, char **argv, Options &opts) {
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'd' && !opts.debug) { // Enable debug mode
        opts.debug = true;
      } else if (argv[i][1] == 'o' && !opts.output_file) { // Provide output file
        i++;

        if (i == argc) {
          std::cout << "-o: expected file path\n";
          return EXIT_FAILURE;
        }

        opts.output_file = argv[i];
      } else if (argv[i][1] == 'l' && !opts.output_lexed) {
        opts.output_lexed = true;
      } else if (argv[i][1] == 'p' && !opts.output_parsed) {
        opts.output_parsed = true;
      } else if (argv[i][1] == 'm' && !opts.output_modules) {
        opts.output_modules = true;
      } else if (argv[i][1] == '-' && !opts.output_dir && strcmp(argv[i] + 2, "out-dir") == 0) {
        i++;

        if (i == argc) {
          std::cout << "--out-dir: expected directory\n";
          return EXIT_FAILURE;
        }

        opts.output_dir = argv[i];
      } else {
        std::cout << "Unknown/repeated flag " << argv[i] << "\n";
        return EXIT_FAILURE;
      }
    } else {
      opts.input_files.emplace_back(argv[i]);
    }
  }

  // Check if at least one input file was provided
  if (opts.input_files.empty()) {
    std::cout << "Expected input file to be provided\n";
    return EXIT_FAILURE;
  }

  // check that output file is provided
  if (opts.output_file == nullptr) {
    std::cout << "Expected output file to be provided (-o <file>)\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/** Attempt to write a stream's contents to the given file. Print if failure. */
bool write_stream_to_file(const std::filesystem::path &filename, const std::ostream &stream) {
  std::ofstream file;
  file.open(filename, std::ios::out);

  if (!file.is_open()) {
    std::cout << "Unable to open file " << filename << " for write." << std::endl;
    return false;
  }

  file << stream.rdbuf();
  file.close();

  return true;
}

/** Given a file and an output directory path, output he filepath of the output file. E.g., (".\dir\a.txt", ".\dir\out") -> ".\dir\out\a.txt" */
std::filesystem::path
get_output_file_path(const std::filesystem::path &input_path, const std::filesystem::path &output_dir) {
  std::string filepath = input_path.string(), directory = output_dir.string();
  if (directory.back() != '\\') directory.push_back('\\');

  // Extract output filename
  auto slash_pos = filepath.find_last_of('\\');
  std::string file = slash_pos == std::string::npos ? filepath : filepath.substr(slash_pos + 1);

  std::string output;

  while (true) {
    slash_pos = filepath.find('\\');
    if (slash_pos == std::string::npos) break;

    std::string subdir = filepath.substr(0, slash_pos + 1);

    if (subdir == directory.substr(0, slash_pos + 1)) {
      output += subdir;
      filepath = filepath.substr(slash_pos + 1);
      directory = directory.substr(slash_pos + 1);
    } else {
      break;
    }
  }

  return output + directory + file;
}

template<typename T>
void delete_vector(std::vector<T *> &vector) {
  for (T *item: vector) {
    delete item;
  }
}

int main(int argc, char *argv[]) {
  // Parse provided options
  Options opts;

  if (parse_arguments(argc, argv, opts) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Create output directory?
  if (opts.output_dir && !std::filesystem::exists(opts.output_dir)) {
    std::filesystem::create_directory(opts.output_dir);
  }

  std::vector<language::CompiledProgram *> modules;

  // Go through input files...
  for (const auto input_file: opts.input_files) {
    // Create source
    language::Source *source = language::Source::read_file(input_file);

    if (source == nullptr) {
      std::cout << "unable to read file " << input_file << std::endl;
      delete_vector(modules);
      return EXIT_FAILURE;
    } else {
      std::cout << "processing file " << input_file << "..." << std::endl;
    }

    auto output_file = opts.output_dir ? get_output_file_path(input_file, opts.output_dir) : input_file;
    message::List messages;

    // Lex the given files
    language::lexer::Lexer lexer(source);

    lexer.lex(messages);

    if (message::print_and_check(messages)) {
      delete source;
      delete_vector(modules);
      return EXIT_FAILURE;
    }

    // Write lexed output?
    if (opts.output_lexed) {
      auto lexed_file = output_file;
      lexed_file.replace_extension("lexed.xml");

      std::cout << "  lexed file: " << lexed_file << std::endl;
      std::stringstream stream;
      lexer.debug_print(stream);

      write_stream_to_file(lexed_file, stream);
    }

    // Parse result
    language::Program program(source);
    language::parser::Parser parser(&program);

    parser.parse(messages);

    if (message::print_and_check(messages)) {
      delete source;
      delete_vector(modules);
      return EXIT_FAILURE;
    }

    if (opts.output_parsed) {
      auto parsed_file = output_file;
      parsed_file.replace_extension("parsed.xml");

      std::cout << "  parsed file: " << parsed_file << std::endl;
      std::stringstream stream;
      program.debug_print(stream);

      write_stream_to_file(parsed_file, stream);
    }

    // Compile program
    /*
    auto *compiled = program.compile(messages);

    if (message::print_and_check(messages)) {
      delete source;
      delete_vector(modules);
      return EXIT_FAILURE;
    }

    if (opts.output_modules) {
      auto module_file = output_file;
      module_file.replace_extension("module.xml");

      std::cout << "  module file: " << module_file << std::endl;
      std::stringstream stream;
      compiled->debug_print(stream);

      write_stream_to_file(module_file, stream);
    }
     */

    // TODO link

    delete source;
    std::cout << "  Done." << std::endl;
  }

  delete_vector(modules);

  return EXIT_SUCCESS;
}
