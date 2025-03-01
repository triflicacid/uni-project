#include <iostream>
#include <vector>
#include <cstring>
#include "named_fstream.hpp"
#include "config.hpp"
#include "messages/list.hpp"
#include "parser.hpp"
#include "assembly/program.hpp"
#include "memory/stack.hpp"
#include "memory/reg_alloc.hpp"
#include "ast/types/int.hpp"
#include "context.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/float.hpp"
#include "operators/builtins.hpp"

struct Options {
  std::unique_ptr<named_fstream> input; // input file
  std::unique_ptr<named_fstream> output; // output file
  bool print_ast = false;
};

int parse_arguments(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "--ast")) { // print ast
        options.print_ast = true;
      } else if (argv[i][1] == 'd') { // debug
        lang::conf::debug = true;
      } else if (!strcmp(argv[i], "--function-placeholder")) { // conf::function_placeholder = true
        lang::conf::function_placeholder = true;
      } else if (!strcmp(argv[i], "--no-function-placeholder")) { // conf::function_placeholder = false
        lang::conf::function_placeholder = false;
      } else if (!strcmp(argv[i], "--indentation")) { // conf::indent_asm_code = true
        lang::conf::indent_asm_code = true;
      } else if (!strcmp(argv[i], "--no-indentation")) { // conf::indent_asm_code = false
        lang::conf::indent_asm_code = false;
      } else if (!strcmp(argv[i], "--always-define-symbols")) { // conf::always_define_symbols = true
        lang::conf::always_define_symbols = true;
      } else if (!strcmp(argv[i], "--lint")) { // enable linting
        lang::conf::lint = true;
      } else if (!strcmp(argv[i], "--no-lint")) { // disable linting
        lang::conf::lint = false;
      } else if (!strcmp(argv[i], "--lint-level")) { // set linting level
        if (++i >= argc) {
          std::cerr << argv[i - 1] << ": expected an integer reporting level" << std::endl;
          return EXIT_FAILURE;
        }

        lang::conf::lint_level = message::level_from_int(std::stoi(argv[i]));
        i++;
      } else if (argv[i][1] == 'o') { // output file
        if (options.output) {
          std::cerr << argv[i] << ": an output file was already provided" << std::endl;
          return EXIT_FAILURE;
        }

        if (++i >= argc) {
          std::cerr << argv[i-1] << ": expected file path to follow flag" << std::endl;
          return EXIT_FAILURE;
        }

        if (auto stream = named_fstream::open(argv[i], std::ios::out)) {
          options.output = std::move(stream);
        } else {
          std::cerr << argv[i-1] << ": failed to open file " << argv[i] << std::endl;
          return EXIT_FAILURE;
        }
      } else {
        std::cerr << "unknown flag " << argv[i] << std::endl;
        return EXIT_FAILURE;
      }
    } else {
      if (options.input) {
        std::cerr << "positional: input file was already provided (found " << argv[i] << ")" << std::endl;
        return EXIT_FAILURE;
      }

      if (auto stream = named_fstream::open(argv[i], std::ios::in)) {
        options.input = std::move(stream);
      } else {
        std::cerr << "positional: failed to open file " << argv[i] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  // check if an input file was provided
  if (!options.input) {
    std::cerr << "no input file provided" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
  Options options;
  if (int code = parse_arguments(argc, argv, options); code != EXIT_SUCCESS) {
    return code;
  }

  // initialise compilation context
  lang::assembly::Program program("main");
  lang::memory::StackManager stack(program);
  lang::symbol::SymbolTable symbol_table(stack);
  lang::memory::RegisterAllocationManager allocation_manager(symbol_table, program);
  message::List messages;
  lang::Context ctx{
      .messages = messages,
      .program = program,
      .stack_manager = stack,
      .reg_alloc_manager = allocation_manager,
      .symbols = symbol_table
  };

  // initialise type dependency graph
  lang::ast::type::TypeGraph::init();

  // initialise builtin operators
  lang::ops::init_builtins();

  // create IStreamWrapper to read the input file
  IStreamWrapper wrapper(std::move(*options.input->take()));
  wrapper.set_name(options.input->path);

  // create lexer & parser objects
  lang::lexer::Lexer lexer(wrapper);
  lang::parser::Parser parser(lexer);
  parser.messages(&messages);

//    while (true) {
//      auto token = lexer.next();
//      std::cout << "Token@<" << token.source.line() << ":" << token.source.column() << "> " << token.to_string() << std::endl;
//      if (token.is_eof()) std::exit(1);
//    }

  // parse the file & check for any errors
  auto ast = parser.parse();
  if (message::print_and_check(*parser.messages(), std::cerr)) {
    return EXIT_FAILURE;
  }

  // print the AST program structure if desired
  if (options.print_ast) {
    ast->print_tree(std::cout) << std::endl;
  }

  // process tree, ensure everything is OK
  ast->process(ctx);
  if (message::print_and_check(messages, std::cerr)) {
    return EXIT_FAILURE;
  }

  // generate assembly code
  ast->generate_code(ctx);
  if (message::print_and_check(messages, std::cerr)) {
    return EXIT_FAILURE;
  }

  // write to output file is provided
  if (options.output) {
    if (lang::conf::debug) {
      options.output->stream << "; debug: on" << std::endl; // mark with tag for visualiser
    }

    ctx.program.print(options.output->stream);
  }

  return EXIT_SUCCESS;
}
