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

static std::ostream *debug_stream = nullptr;

static void handle_debug_message(const processor::debug::Message &msg) {
  using namespace processor::debug;

  switch (msg.type) {
    case Message::Cycle: {
      auto *message = (CycleMessage*)&msg;
      *debug_stream << ANSI_VIOLET;
      if (message->n < 0) *debug_stream << "step";
      else *debug_stream << "cycle #" << message->n;
      *debug_stream << ANSI_RESET ": $pc=0x" << std::hex << message->pc << ", inst=0x" << message->inst << std::dec << std::endl;
      break;
    }
    case Message::Instruction: {
      auto *message = (InstructionMessage*)&msg;
      *debug_stream << message->mnemonic << ": " << message->message.str() << std::endl;
      break;
    }
    case Message::Argument: {
      auto *message = (ArgumentMessage*)&msg;
      *debug_stream << ANSI_BLUE "arg #" << message->n << ANSI_RESET ": " ANSI_CYAN << constants::inst::arg_to_string(message->arg_type) << ANSI_RESET << message->message.str() << ANSI_CYAN " resolved to " ANSI_RESET << "0x" << std::hex << message->value << std::dec << std::endl;
      break;
    }
    case Message::Register: {
      auto *message = (RegisterMessage*)&msg;
      *debug_stream << ANSI_BRIGHT_YELLOW "reg" ANSI_RESET ": ";
      if (message->is_write) *debug_stream << "set $" << constants::registers::to_string(message->reg) << " to ";
      else *debug_stream << "access $" << constants::registers::to_string(message->reg) << " -> ";
      *debug_stream << "0x" << std::hex << message->value << std::dec << std::endl;
      break;
    }
    case Message::Memory: {
      auto *message = (MemoryMessage*)&msg;
      if (message->is_write) *debug_stream << ANSI_YELLOW "mem" ANSI_RESET ": store data 0x" << std::hex << message->value << " of " << std::dec << (int) message->bytes << " bytes at address 0x" << std::hex << message->address << std::dec << std::endl;
      else *debug_stream << ANSI_YELLOW "mem" ANSI_RESET ": access " << (int) message->bytes << " bytes from address 0x" << std::hex << message->address << " -> 0x" << message->value << std::dec << std::endl;
      break;
    }
    case Message::ZeroFlag: {
      auto *message = (ZeroFlagMessage*)&msg;
      *debug_stream << ANSI_CYAN "zero flag" ANSI_RESET ": register $" << constants::registers::to_string(message->reg) << " -> " << (message->state ? ANSI_GREEN "set" : ANSI_RED "reset") << ANSI_RESET << std::endl;
      break;
    }
    case Message::Conditional: {
      auto *message = (ConditionalMessage*)&msg;
      *debug_stream << ANSI_CYAN "conditional" ANSI_RESET ": " << constants::cmp::to_string(message->test_bits) << " -> ";
      if (message->passed) *debug_stream << ANSI_GREEN "pass" ANSI_RESET << std::endl;
      else *debug_stream << ANSI_RED "fail" ANSI_RESET " ($flag: 0x" << std::hex << (int) message->flag_bits.value() << std::dec << ")" << std::endl;
      break;
    }
    case Message::Interrupt: {
      auto *message = (InterruptMessage*)&msg;
      *debug_stream << ANSI_CYAN "interrupt! " ANSI_RESET
             "$isr=0x" << std::hex << message->isr << ", $imr=0x" << message->imr << ", %iip=0x" << message->ipc << std::dec << std::endl;
      break;
    }
    case Message::Error: {
      auto *message = (ErrorMessage*)&msg;
      *debug_stream << ANSI_RED << message->message << ANSI_RESET << std::endl;
      break;
    }
  }
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
  debug_stream = args.debug_file ? &args.debug_file->stream : &std::cout;

  // reset the processor
  cpu.reset();

  // determine file size
  auto &stream = args.source_file->stream;
  stream.seekg(-1, std::ios::end);
  size_t file_size = stream.tellg();
  stream.seekg(std::ios::beg);

  if (debug::cpu)
    *debug_stream << "reading source file " << args.source_file->path << "... " << file_size << " bytes read" << std::endl;

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

  // print debug messages
  for (const auto &m : cpu.get_debug_messages())
    handle_debug_message(*m);

  // print error (if any) and notify user of exit code
  cpu.print_error(true);

  auto err_code = cpu.get_error();
  uint64_t code = err_code ? err_code : cpu.get_return_value();
  if (debug::cpu) *debug_stream << "processor exited with code " << code << std::endl;

  return EXIT_SUCCESS;
}
