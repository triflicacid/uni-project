#include <iostream>
#include <memory>
#include <vector>
#include "named_fstream.hpp"
#include "config.hpp"
#include "lexer.hpp"
#include "shell.hpp"
#include "messages/list.hpp"
#include "parser.hpp"

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

    messages.add(std::move(parser.generate_syntax_error({lang::lexer::TokenType::ident})));
    if (parser.is_error()) {
      handle_messages(messages);
    }

    while (true) {
      auto token = parser.consume();
      std::cout << "Token@<" << token.source.path() << ":" << token.source.line() << ":" << token.source.column() << "> " << ANSI_GREEN << token.image << ANSI_RESET << std::endl;
      if (token.is_eof()) break;
    }


  }

  return EXIT_SUCCESS;
}
