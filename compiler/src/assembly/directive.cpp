#include "directive.hpp"
#include <sstream>

std::ostream& lang::assembly::Directive::_print(std::ostream& os) const {
  return os << "." << name_;
}

std::unique_ptr<lang::assembly::BytesDirective> lang::assembly::Directive::bytes() {
  return std::make_unique<BytesDirective>("byte");
}

std::unique_ptr<lang::assembly::StringDirective> lang::assembly::Directive::string(const std::string& str) {
  auto dir = std::make_unique<StringDirective>("byte");
  dir->add(str);
  return dir;
}

std::unique_ptr<lang::assembly::DataDirective> lang::assembly::Directive::data() {
  return std::make_unique<DataDirective>("data");
}

std::unique_ptr<lang::assembly::WordDirective> lang::assembly::Directive::words() {
  return std::make_unique<WordDirective>("word");
}

std::unique_ptr<lang::assembly::Directive> lang::assembly::Directive::space(uint32_t n) {
  return std::make_unique<_SingleDirective>("space", n);
}

std::unique_ptr<lang::assembly::Directive> lang::assembly::Directive::offset(uint32_t n) {
  return std::make_unique<_SingleDirective>("offset", n);
}

std::ostream& lang::assembly::_SingleDirective::_print(std::ostream& os) const {
  return Directive::_print(os) << " 0x" << std::hex << n_ << std::dec;
}

lang::assembly::BytesDirective& lang::assembly::BytesDirective::add(char x) {
  data_.push_back(*(uint8_t*)&x);
  return *this;
}

lang::assembly::BytesDirective& lang::assembly::BytesDirective::add(const std::string& str) {
  for (char ch : str)
    add(ch);
  return *this;
}

std::ostream& lang::assembly::StringDirective::_print(std::ostream& os) const {
  Directive::_print(os);

  std::deque<std::pair<bool, std::stringstream>> strings; // string segments - either substring, or sequence of "0x.., 0x.. ..."
  bool is_string = true;
  strings.emplace_back(is_string, std::stringstream());

  for (char c : data_) {
    // check if the current character is printable, add new stream if necessary
    bool is_string_now = isprint(c);
    if (is_string != is_string_now) strings.emplace_back(is_string_now, std::stringstream{});

    // print correct thing to the stream
    if ((is_string = is_string_now)) {
      strings.back().second << c;
    } else {
      strings.back().second << (strings.back().second.str().empty() ? "" : " ") << "0x" << std::hex << (int) c << std::dec;
    }
  }

  // now, print each substring
  for (const auto& [is_string, stream] : strings) {
    const std::string str = stream.str();
    if (str.empty()) continue;
    if (is_string) {
      os << " \"" << str << "\"";
    } else {
      os << " " << str;
    }
  }

  // print terminating byte
  return os << " 0x0";
}
