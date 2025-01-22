#include <iostream>
#include <memory>
#include <vector>
#include "named_fstream.hpp"
#include "config.hpp"
#include "messages/list.hpp"
#include "parser.hpp"
#include "assembly/program.hpp"
#include "assembly/create.hpp"
#include "assembly/arg.hpp"
#include "assembly/directive.hpp"
#include "memory/stack.hpp"
#include "memory/reg_alloc.hpp"
#include "ast/types/int.hpp"
#include "context.hpp"

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

/** Handle message list: debug_print messages and empty the list, return if there was an error. */
bool handle_messages(message::List &list) {
  list.for_each_message([](message::Message &msg) {
    msg.print(std::cerr);
  });

  bool is_error = list.has_message_of(message::Error);
  list.clear();
  return is_error;
}

int main(int argc, char** argv) {
//  using namespace lang;
//  assembly::Program program("start");
//  memory::StackManager stack(program);
//  symbol::SymbolTable symbol_table(stack);
//  memory::RegisterAllocationManager allocation_manager(symbol_table, program);
//
//  const std::string name = "pi";
//  lexer::Token token{.type=lexer::TokenType::ident, .image=name, .source=Location("C:/fakepath/file.txt", 1, 1)};
//  symbol_table.insert(std::make_unique<symbol::Variable>(token, ast::type::int32));
//
//  auto& var = static_cast<symbol::Variable&>(*(*symbol_table.find(name))[0]);
//  allocation_manager.find(var);
//
//  token = {.type=lexer::TokenType::int_lit, .image="7", .source=token.source, .value=7};
//  ast::expr::LiteralNode literal(token, ast::type::int32);
//  allocation_manager.find(literal);
//
//  program.print(std::cout);
//
//  return EXIT_SUCCESS;

  Options options;
  if (int code = parse_arguments(argc, argv, options); code != EXIT_SUCCESS) {
    return code;
  }

  // initialise compilation context
  lang::assembly::Program program("start");
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

  // for each source...
  for (std::unique_ptr<named_fstream>& file : options.files) {
    // wrap IStreamWrapper around the file's input stream
    IStreamWrapper wrapper(std::move(*file->take()));
    wrapper.set_name(file->path);

    // create lexer & parser objects
    lang::lexer::Lexer lexer(wrapper);
    lang::parser::Parser parser(lexer);
    parser.messages(&messages);

    // parse the file & check for any errors
    auto ast = parser.parse_func();
    if (!parser.is_error()) parser.expect_or_error(lang::lexer::TokenType::eof);
    if (handle_messages(*parser.messages())) {
      return EXIT_FAILURE;
    }

    ast->print_tree(std::cout);
    std::cout << std::endl << std::endl;
    ast->print_code(std::cout);

    // process into assembly
    ast->process(ctx);
    if (handle_messages(messages)) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
