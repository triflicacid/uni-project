#include "istream_wrapper.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

IStreamWrapper::IStreamWrapper(std::ifstream stream) {
  istream = std::make_unique<std::ifstream>(std::move(stream));
  initialise();
}

IStreamWrapper::IStreamWrapper(std::istringstream stream) {
  istream = std::make_unique<std::istringstream>(std::move(stream));
  initialise();
}

IStreamWrapper::IStreamWrapper(const std::string& source) {
  istream = std::make_unique<std::istringstream>(source);
  initialise();
}

void IStreamWrapper::initialise() {
  // set initial state from stream
  pos.line = 1;
  pos.col = 1;
  pos.stream = istream->tellg();

  // push to cached stack - stack should always have this one item
  positions.push_back(pos);
}

void IStreamWrapper::set_position(const Position& pos) {
  this->pos = pos;
  istream->clear();
  istream->seekg((int) pos.stream);
}

const IStreamWrapper::Position& IStreamWrapper::get_position() const {
  return pos;
}

const IStreamWrapper::Position& IStreamWrapper::prev_position() const {
  return positions.back();
}

const IStreamWrapper::Position& IStreamWrapper::save_position() {
  Position pos = get_position();
  istream->clear();
  pos.stream = istream->tellg();
  positions.push_back(pos);
  return positions.back();
}

void IStreamWrapper::restore_position() {
  if (positions.empty()) {
    return;
  }

  set_position(positions.back());

  // only discard if not the base item
  if (positions.size() > 1) {
    positions.pop_back();
  }
}

void IStreamWrapper::discard_position() {
  if (positions.size() < 2) {
    return;
  }

  istream->clear();
  positions.pop_back();
}

void IStreamWrapper::reset_position() {
  // revert to initial state (base item in stack)
  const auto initialPosition = positions.front();
  positions.clear();
  positions.push_back(initialPosition);
  set_position(initialPosition);
}

bool IStreamWrapper::starts_with(const std::string& s) {
  if (s.size() == 1) {
    if (istream->peek() == s[0]) {
      get_char();
      return true;
    }

    return false;
  }

  // read s.size chars from stream into a buffer
  istream->clear();
  save_position();
  std::string buffer(s.size(), '\0');
  istream->read(&buffer[0], s.size());

  // return if match (restore old position if not)
  if (buffer == s) {
    discard_position();
    pos.stream += s.size();
    pos.col += s.size();
    return true;
  }

  restore_position();
  return false;
}

std::string IStreamWrapper::extract(unsigned int n) {
  std::string result(n, '\0');
  istream->read(&result[0], n);
  pos.stream += result.size();
  return result;
}

std::string IStreamWrapper::get_line(unsigned lineNo) {
  save_position();
  istream->clear();
  istream->seekg(0);
  std::string line;
  unsigned n = 0;

  while (std::getline(*istream, line)) {
    if (++n == lineNo) {
      break;
    }
  }

  restore_position();
  return line;
}

bool IStreamWrapper::is_eof() const {
  if (istream->eof() || istream->peek() == EOF) {
    istream->clear();
    return true;
  }

  return false;
}

void IStreamWrapper::eat_whitespace() {
  while (!is_eof() && std::isspace(istream->peek())) {
    get_char();
  }
}

int IStreamWrapper::get_char() {
  if (is_eof()) {
    return EOF;
  }

  // get character from stream, move positions
  const int ch = istream->get();
  istream->clear();

  if ((ch == '\r' && istream->peek() == '\n') || ch == '\n') {
    if (ch == '\r') {
      istream->get();
    }

    pos.line++;
    pos.col = 1;
  } else if (ch == EOF) {
    istream->seekg(0, std::ios::end);
  } else {
    pos.col++;
  }

  pos.stream = istream->tellg();

  return ch;
}

void IStreamWrapper::advance(unsigned n) {
  if (n == 1) {
    get_char();
    return;
  }

  for (int i = 0; i < n && !is_eof(); i++) {
    get_char();
  }
}

int IStreamWrapper::peek_char() const {
  int ch = istream->peek();
  istream->clear();
  return ch;
}

void IStreamWrapper::eat_while(const std::function<bool(int)>& pred) {
  while (!is_eof() && pred(istream->peek())) {
    get_char();
  }
}

void IStreamWrapper::eat_while(std::ostream& os, const std::function<bool(int)>& pred) {
  while (!is_eof() && pred(istream->peek())) {
    os << (char) get_char();
  }
}

void IStreamWrapper::eat_line() {
  int oldLine = pos.line;
  while (pos.line == oldLine) {
    get_char();
  }
}
