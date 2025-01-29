#include <iostream>

#include "message.hpp"
#include "shell.hpp"

void message::BasicMessage::print_type_suffix(std::ostream& os) const {
  switch (level_) {
    case Level::Note:
      os << ANSI_BLUE "note" ANSI_RESET;
      break;
    case Level::Warning:
      os << ANSI_YELLOW "warning" ANSI_RESET;
      break;
    case Level::Error:
      os << ANSI_RED "error" ANSI_RESET;
      break;
  }

  if (int code = get_code(); code != -1) {
    std::cout << " (" << code << ")";
  }
}

void message::BasicMessage::print(std::ostream& os) const {
  print_type_suffix(os);

  std::string msg = msg_.str();
  if (!msg.empty() && msg.back() == '\r') msg = msg.substr(0, msg.size() - 1);
  os << ": " << msg << std::endl;
}

void message::Message::print(std::ostream& os) const {
  loc_.print(os) << ": ";
  BasicMessage::print(os);
}

message::Level message::level_from_int(int level) {
  if (level < 1) return Level::Note;
  if (level == 1) return Level::Warning;
  return Level::Error;
}
