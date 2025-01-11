#pragma once

#include <memory>
#include <deque>
#include <istream>
#include <functional>
#include <optional>

class IStreamWrapper {
public:
  struct Position {
    std::streampos stream;
    int line;
    int col;
  };

private:
  std::deque<Position> positions;
  Position pos; // doesn't use .stream here, only in cache
  std::unique_ptr<std::istream> istream;
  std::optional<std::string> name;

public:
  explicit IStreamWrapper(std::ifstream stream);
  explicit IStreamWrapper(std::fstream stream);
  explicit IStreamWrapper(std::istringstream stream);
  explicit IStreamWrapper(const std::string& source);

  void initialise();

  [[nodiscard]] std::string get_name(const std::string& fallback) const { return name.value_or(fallback); }

  void set_name(const std::string& name) { this->name = name; }

  // get current position
  [[nodiscard]] const Position& get_position() const;

  // set the current position
  void set_position(const Position& pos);

  // get the previous position
  [[nodiscard]] const Position& prev_position() const;

  // reset entire stream position
  void reset_position();

  // save stream position
  const Position& save_position();

  // restore old position
  void restore_position();

  // discard old position
  void discard_position();

  // check if the stream starts with...
  [[nodiscard]] bool starts_with(const std::string& s);

  // getFormat first `n` characters from the stream
  std::string extract(unsigned n);

  // get the `n`th line starting from 1
  std::string get_line(unsigned lineNo);

  // eat whitespace form current position (uses `std::isspace`)
  void eat_whitespace();

  // eat characters while the predicate matches
  void eat_while(const std::function<bool(int)>& pred);

  // eat characters while the predicate matches
  void eat_while(std::ostream& os, const std::function<bool(int)>& pred);

  // eat the rest of this line
  void eat_line();

  // check if isEof has been reached - this is faster than ::nextToken().isEof()
  [[nodiscard]] bool is_eof() const;

  // read the next character from the stream
  int get_char();

  // advance by `n` characters, default 1
  void advance(unsigned n = 1);

  // peek next chara ter from stream
  [[nodiscard]] int peek_char() const;
};
