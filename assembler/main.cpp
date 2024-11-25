#include <cstdlib>
#include <string>
#include <iostream>
#include <cstring>

#include "assembler/src/cli_arguments.hpp"
#include "src/pre-process/pre-processor.hpp"
#include "messages/list.hpp"
#include "assembler_data.hpp"
#include "parser.hpp"

/** Handle message list: debug_print messages and empty the list, return if there was an error. */
bool handle_messages(message::List &list) {
  list.for_each_message([](message::Message &msg) {
    msg.print(std::cerr);
  });

  bool is_error = list.has_message_of(message::Error);
  list.clear();
  return is_error;
}

/** Parse command-line arguments. */
int parse_arguments(int argc, char **argv, assembler::CliArguments &opts) {
  std::string lib_path_suffix = "lib";

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'd' && !opts.debug) {
        // Enable debug mode
        opts.debug = true;
      } else if (argv[i][1] == 'o' && !opts.output_file) {
        // Provide output file
        if (++i == argc) {
          std::cout << "-o: expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::out | std::ios::binary)) {
          opts.output_file = std::move(stream);
        } else {
          std::cout << "-o: failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (argv[i][1] == 'p' && !opts.post_processing_file) {
        // Provide post-process output file
        if (++i >= argc) {
          std::cout << "-p: expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::out)) {
          opts.post_processing_file = std::move(stream);
        } else {
          std::cout << "-p: failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (argv[i][1] == 'r' && !opts.reconstructed_asm_file) {
        // Provide file to write reconstructed asm to
        if (++i >= argc) {
          std::cout << "-r: expected file path\n";
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::out)) {
          opts.reconstructed_asm_file = std::move(stream);
        } else {
          std::cout << "-r: failed to open file " << argv[i];
          return EXIT_FAILURE;
        }
      } else if (argv[i][1] == 'l') {
        // Provide library path
        lib_path_suffix = ++i < argc ? argv[i] : "";
      } else if (opts.do_pre_processing && strcmp(argv[i] + 1, "-no-pre-process") == 0) {
        // Skip pre-processing
        opts.do_pre_processing = false;
      } else if (opts.do_compilation && strcmp(argv[i] + 1, "-no-compile") == 0) {
        // Skip compilation
        opts.do_compilation = false;
      } else {
        std::cout << "Unknown/repeated flag " << argv[i] << "\n";
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
      std::cout << "Unexpected argument '" << argv[i] << "'\n";
      return EXIT_FAILURE;
    }
  }

  // Check if all files are present
  if (!opts.source) {
    std::cout << "Expected input file to be provided\n";
    return EXIT_FAILURE;
  }

  if (!opts.output_file && opts.do_compilation) {
    std::cout << "Expected output file to be provided (-o <file>)\n";
    return EXIT_FAILURE;
  }

  if (opts.debug)
    std::cout << "source file: " << opts.source->path << std::endl
              << "post-processed file: " << (opts.post_processing_file ? opts.post_processing_file->path : "(null)")
              << std::endl
              << "reconstruction file: " << (opts.reconstructed_asm_file ? opts.reconstructed_asm_file->path : "(null)")
              << std::endl
              << "output file: " << opts.output_file->path << std::endl;

  // check that lib directory exists
  opts.lib_path = weakly_canonical(std::filesystem::current_path() / lib_path_suffix);

  if (!exists(opts.lib_path)) {
    std::cout << "library path " << opts.lib_path << " does not exist" << std::endl;
    return EXIT_FAILURE;
  }

  if (opts.debug) std::cout << "library path: " << opts.lib_path << std::endl;
  if (!is_directory(opts.lib_path)) {
    std::cout << "library path " << opts.lib_path << " does not point to a directory" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int pre_process_data(assembler::pre_processor::Data &data, message::List &messages) {
  if (data.cli_args.debug)
    std::cout << ANSI_GREEN "=== PRE-PROCESSING ===\n" ANSI_RESET;

  assembler::pre_process(data, messages);

  // Check if error
  if (handle_messages(messages))
    return EXIT_FAILURE;

  if (data.cli_args.debug) {
    // Print constants
    if (!data.constants.empty()) {
      std::cout << "--- Constants ---\n";

      for (const auto &pair: data.constants) {
        std::cout << "%define " << pair.first << " " << pair.second.value << "\n";
      }
    }

    // Print macros
    if (!data.macros.empty()) {
      std::cout << "--- Macros ---\n";

      for (const auto &pair: data.macros) {
        std::cout << "%macro " << pair.first << " ";

        for (const auto &param: pair.second.params) {
          std::cout << param << " ";
        }

        std::cout << "(" << pair.second.lines.size() << " lines)\n";
      }
    }
  }

  // Write post-processed content to file?
  if (auto &file = data.cli_args.post_processing_file) {
    // Write post-processed content to the output stream
    data.write_lines(file->stream);

    if (data.cli_args.debug)
      std::cout << "Written post-processed source to " << file->path << "\n";
  }

  return EXIT_SUCCESS;
}

int parse_data(assembler::Data &data, message::List &messages) {
  // Parse pre-processed lines
  if (data.cli_args.debug)
    std::cout << ANSI_GREEN "=== PARSING ===\n" ANSI_RESET;

  assembler::parser::parse(data, messages);

  // Check if error
  if (handle_messages(messages)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int compile_result(assembler::Data &data) {
  if (data.cli_args.debug) {
    // Print chunks
    std::cout << ANSI_GREEN "=== COMPILATION ===\n" ANSI_RESET;

    for (const auto &chunk: data.buffer) {
      chunk->print(std::cout);
    }
  }

  // Write compiled chunks to output file
  auto &handle = *data.cli_args.output_file;
  auto before = handle.stream.tellp();
  data.write(handle.stream);
  auto after = handle.stream.tellp();

  if (data.cli_args.debug)
    std::cout << "Written " << after - before + 1 << " bytes to file " << handle.path << "\n";

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  // Parse CLI
  assembler::CliArguments opts;

  if (parse_arguments(argc, argv, opts) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Set-up pre-processing data
  assembler::pre_processor::Data pre_data(opts);
  pre_data.set_executable(argv[0]);
  message::List messages;

  // Read source file into lines
  assembler::read_source_file(pre_data, messages);

  // Check if error
  if (handle_messages(messages))
    return EXIT_FAILURE;

  // Pre-process file
  if (opts.do_pre_processing && pre_process_data(pre_data, messages) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Construct data structure for parsing
  assembler::Data data(pre_data);

  if (parse_data(data, messages) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  // Compile data
  if (opts.do_compilation && compile_result(data) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
