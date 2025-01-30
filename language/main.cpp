#include <iostream>
#include <vector>
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
#include "operators/builtin.hpp"

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

  // for each source...
  for (std::unique_ptr<named_fstream>& file : options.files) {
    // wrap IStreamWrapper around the file's input stream
    IStreamWrapper wrapper(std::move(*file->take()));
    wrapper.set_name(file->path);

    // create lexer & parser objects
    lang::lexer::Lexer lexer(wrapper);
    lang::parser::Parser parser(lexer);
    parser.messages(&messages);

    while (true) {
      auto token = lexer.next();
      std::cout << "Token@<" << token.source.line() << ":" << token.source.column() << "> " << token.to_string() << std::endl;
      if (token.is_eof()) std::exit(1);
    }
    return 1;

    // parse the file & check for any errors
    auto ast = parser.parse();
    //if (!parser.is_error()) parser.expect_or_error(lang::lexer::TokenType::eof);
    if (message::print_and_check(*parser.messages(), std::cerr)) {
      return EXIT_FAILURE;
    }

    ast->print_tree(std::cout) << std::endl;
//    ast->print_code(std::cout) << std::endl;

    // process into assembly
    ast->process(ctx);
    if (message::print_and_check(messages, std::cerr)) {
      return EXIT_FAILURE;
    }
  }

  ctx.program.print(std::cout);

  return EXIT_SUCCESS;
}
