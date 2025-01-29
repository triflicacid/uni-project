#include "MessageWithSource.hpp"
#include "shell.hpp"
#include <string>
#include <iostream>
#include <utility>

namespace message {
  MessageWithSource::MessageWithSource(Level level, Location loc, int idx, int len, const std::string& src)
      : Message(level, std::move(loc)) {
    idx_ = std::max(0, idx);
    len_ = std::max(1, std::min(len, (int) src.length() - idx_));
    src_ = src;
  }

  void MessageWithSource::print_notice() const {
    std::cout << ANSI_BLUE "note" ANSI_RESET " " << loc_.path() << ':' << loc_.line() << ':' << loc_.column() << ": "
              << msg_.str()
              << '\n' << loc_.line() << " | " << src_.substr(0, idx_) << ANSI_BLUE << src_.substr(idx_, len_)
              << ANSI_RESET << src_.substr(idx_ + len_)
              << '\n' << std::string(std::to_string(loc_.line()).length(), ' ') << "   " << std::string(idx_, ' ')
              << ANSI_BLUE "^" << std::string(len_ - 1, '~') << ANSI_RESET
              << std::endl;
  }

  void MessageWithSource::print_warning() const {
    std::cout << ANSI_YELLOW "warning" ANSI_RESET " " << loc_.path() << ':' << loc_.line() << ':' << loc_.column()
              << ": " << msg_.str()
              << '\n' << loc_.line() << " | " << src_.substr(0, idx_) << ANSI_YELLOW << src_.substr(idx_, len_)
              << ANSI_RESET << src_.substr(idx_ + len_)
              << '\n' << std::string(std::to_string(loc_.line()).length(), ' ') << "   " << std::string(idx_, ' ')
              << ANSI_YELLOW "^" << std::string(len_ - 1, '~') << ANSI_RESET
              << std::endl;
  }

  void MessageWithSource::print_error() const {
    std::cout << ANSI_RED "error" ANSI_RESET " " << loc_.path() << ':' << loc_.line() << ':' << loc_.column()
              << ": " ANSI_YELLOW << msg_.str() << ANSI_RESET
              << '\n' << loc_.line() << " | ";

    if (idx_ >= src_.length()) {
      std::cout << src_ << ANSI_RED_BG << std::string(len_, ' ') << ANSI_RESET;
    } else {
      std::cout << src_.substr(0, idx_) << ANSI_RED << src_.substr(idx_, len_) << ANSI_RESET
                << src_.substr(idx_ + len_);
    }


    std::cout << '\n' << std::string(std::to_string(loc_.line()).length(), ' ') << "   " << std::string(idx_, ' ')
              << ANSI_RED "^" << std::string(len_ - 1, '~') << ANSI_RESET
              << std::endl;
  }

  void MessageWithSource::print(std::ostream& os) const {
    switch (level_) {
      case Level::Note:
        print_notice();
        return;
      case Level::Warning:
        print_warning();
        return;
      case Level::Error:
        print_error();
        return;
    }
  }
}
