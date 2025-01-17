#include <iostream>
#include <memory>
#include <vector>
#include "named_fstream.hpp"
#include "config.hpp"
#include "lexer.hpp"
#include "messages/list.hpp"
#include "parser.hpp"
#include "assembly/program.hpp"
#include "assembly/create.hpp"
#include "assembly/arg.hpp"
#include "assembly/directive.hpp"

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
  lang::assembly::Program program("main");

  program.insert(lang::assembly::Position::Before, lang::assembly::BasicBlock::labelled("message"));
  program.current().add(lang::assembly::Directive::string("Current value is "));

  program.select("main");
  program.current().add(lang::assembly::create_load(10, lang::assembly::Arg::imm(69)));

  auto inst = lang::assembly::instruction("print_str");
  inst->add_arg(lang::assembly::Arg::label(program.get("message")));
  program.current().add(std::move(inst));

  inst = lang::assembly::instruction("print_int");
  inst->add_arg(lang::assembly::Arg::reg(10));
  program.current().add(std::move(inst));

  program.current().add(lang::assembly::create_exit());
  program.current().add(lang::assembly::create_branch(lang::assembly::Arg::label(program.current())));

  program.print(std::cout);
  return EXIT_SUCCESS;

  Options options;
  if (int code = parse_arguments(argc, argv, options); code != EXIT_SUCCESS) {
    return code;
  }

  // for each source...
  message::List messages;

  for (std::unique_ptr<named_fstream>& file : options.files) {
    // wrap IStreamWrapper around the file's input stream
    IStreamWrapper wrapper(std::move(*file->take()));
    wrapper.set_name(file->path);

    // create lexer & parser objects
    lang::lexer::Lexer lexer(wrapper);
    lang::parser::Parser parser(lexer);
    parser.messages(&messages);

    auto expr = parser.parse_expression();
    if (!parser.is_error()) parser.expect_or_error({lang::lexer::TokenType::eof});
    if (handle_messages(*parser.messages())) {
      return EXIT_FAILURE;
    }

    expr->print_tree(std::cout);
    expr->process(messages);
    if (handle_messages(messages)) {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
