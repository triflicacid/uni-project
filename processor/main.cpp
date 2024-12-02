#include "cpu.hpp"
#include "debug.hpp"
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
      } else if (arg == "-dout") {
        if (++i >= argc) {
          std::cerr << arg << ": expected file path.";
          return EXIT_FAILURE;
        }

        if (!(args.debug_file = named_fstream::open(argv[i], std::ios::out))) {
          std::cerr << arg << ": failed to open file '" << argv[i] << "'";
          return EXIT_FAILURE;
        }
      } else if (arg == "--halt-on-nop") {
        if (++i >= argc) {
          std::cerr << arg << ": expected a yes or no.";
          return EXIT_FAILURE;
        }

        arg = argv[i];
        if (arg == "yes" || arg == "y" || arg == "Y") {
          constants::halt_on_nop = true;
        } else if (arg == "no" || arg == "n" || arg == "N") {
          constants::halt_on_nop = false;
        } else {
          std::cerr << arg << ": expected 'yes' or 'no'.";
          return EXIT_FAILURE;
        }
      } else if (arg == "-dall") {
        processor::debug::set_all(true);
      } else if (arg == "-dargs") {
        processor::debug::args = !processor::debug::args;
      } else if (arg == "-dcpu") {
        processor::debug::cpu = !processor::debug::cpu;
      } else if (arg == "-dmem") {
        processor::debug::mem = !processor::debug::mem;
      } else if (arg == "-dzflag") {
        processor::debug::zflag = !processor::debug::zflag;
      } else if (arg == "-dcond") {
        processor::debug::conditionals = !processor::debug::conditionals;
      } else if (arg == "-derr") {
        processor::debug::errs = !processor::debug::errs;
      } else if (arg == "-dreg") {
        processor::debug::reg = !processor::debug::reg;
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
  debug::set_all(false);
  CliArguments args;

  if (parse_arguments(argc, argv, args) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  // initialise CPU and its streams
  CPU cpu;
  if (args.output_file) cpu.os = &args.output_file->stream;
  if (args.input_file) cpu.is = &args.input_file->stream;
  if (args.debug_file) cpu.ds = &args.debug_file->stream;

  // reset the processor
  cpu.reset();

  // determine file size
  auto &stream = args.source_file->stream;
  stream.seekg(-1, std::ios::end);
  size_t file_size = stream.tellg();
  stream.seekg(std::ios::beg);

  if (debug::cpu)
    *cpu.ds << "reading source file " << args.source_file->path << "... " << file_size << " bytes read" << std::endl;

  // error if file size exceeds buffer size
  if (file_size >= dram::size) {
    std::cerr << ERROR_STR "source file size of " << file_size << " bytes exceeds memory size of " << dram::size
              << std::endl;
    return EXIT_FAILURE;
  }

  // instantiate from file
  read_binary_file(cpu, stream);

  // start processor
  cpu.step_cycle();

  // print error (if any) and notify user of exit code
  cpu.print_error(true);

  auto err_code = cpu.get_error();
  uint64_t code = err_code ? err_code : cpu.get_return_value();
  if (debug::cpu) *cpu.ds << "processor exited with code " << code << std::endl;

  return EXIT_SUCCESS;
}
