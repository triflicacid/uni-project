#pragma once

#include <memory>
#include <deque>
#include <istream>
#include <functional>
#include <optional>
#include <map>

/**
 * @brief A seekable, position-tracking wrapper around an input stream, used to read source text character by character.
 *
 * Owns (or is handed) an `std::istream` and tracks line/column position alongside a stack of
 * saved positions, so a caller can speculatively read ahead and roll back (`save_position`/`restore_position`).
 * Also caches whole lines on first access for fast re-reading (e.g. for error message context).
 */
class IStreamWrapper {
public:
  /** @brief A position within the wrapped stream: byte offset plus 1-based line and column. */
  struct Position {
    std::streampos stream; ///< Byte offset into the wrapped stream.
    int line; ///< 1-based line number.
    int col; ///< 1-based column number.
  };

private:
  std::deque<Position> positions; ///< Stack of positions saved via @ref save_position, restored/discarded LIFO.
  Position pos; ///< Current read position; doesn't use `.stream` here, only in cache lookups.
  std::unique_ptr<std::istream> istream; ///< The wrapped input stream.
  std::optional<std::string> name; ///< Display name for the stream (e.g. source file path), if set.
  std::map<unsigned int, std::string> lines; ///< Cache of whole-line text, keyed by 1-based line number.

public:
  /** @brief Wrap an input file stream. @param stream Stream to take ownership of. */
  explicit IStreamWrapper(std::ifstream stream);
  /** @brief Wrap a file stream. @param stream Stream to take ownership of. */
  explicit IStreamWrapper(std::fstream stream);
  /** @brief Wrap a string stream. @param stream Stream to take ownership of. */
  explicit IStreamWrapper(std::istringstream stream);
  /** @brief Wrap a string as an in-memory stream. @param source Text to read from. */
  explicit IStreamWrapper(const std::string& source);

  /** @brief Set up initial position tracking; called once by every constructor. */
  void initialise();

  /**
   * @brief Get the stream's display name.
   * @param fallback Name to return if none was set.
   * @return The name set via @ref set_name, or `fallback`.
   */
  [[nodiscard]] std::string get_name(const std::string& fallback) const { return name.value_or(fallback); }

  /**
   * @brief Set the stream's display name (e.g. the source file path).
   * @param name Name to associate with this stream.
   */
  void set_name(const std::string& name) { this->name = name; }

  /** @brief Get the current read position. @return Current position. */
  [[nodiscard]] const Position& get_position() const;

  /**
   * @brief Seek the stream to a given position.
   * @param pos Position to move to.
   */
  void set_position(const Position& pos);

  /** @brief Get the most recently saved position. @return The last-saved position on the position stack. */
  [[nodiscard]] const Position& prev_position() const;

  /** @brief Reset the stream back to its initial position, discarding all saved positions. */
  void reset_position();

  /** @brief Save the current position onto the position stack. @return The saved position. */
  const Position& save_position();

  /** @brief Seek back to the most recently saved position, popping it off the stack (unless it's the base position). */
  void restore_position();

  /** @brief Discard the most recently saved position without seeking back to it. */
  void discard_position();

  /**
   * @brief Check whether the stream's upcoming text matches `s`, consuming it if so.
   * @param s Text to match against.
   * @return True if the stream started with `s` (and it was consumed); false otherwise (position unchanged).
   */
  [[nodiscard]] bool starts_with(const std::string& s);

  /**
   * @brief Read and consume the next `n` characters.
   * @param n Number of characters to read.
   * @return The characters read.
   */
  std::string extract(unsigned n);

  /**
   * @brief Get the text of a specific line, without moving the current read position.
   * @param lineNo 1-based line number.
   * @return The line's text (cached after first access).
   */
  std::string get_line(unsigned lineNo);

  /**
   * @brief Consume whitespace characters from the current position.
   * @param exclude_newline If true, stop before consuming a '\\r' or '\\n'.
   */
  void eat_whitespace(bool exclude_newline = false);

  /**
   * @brief Consume characters while a predicate matches.
   * @param pred Called with each upcoming character; consumption stops on the first false result.
   */
  void eat_while(const std::function<bool(int)>& pred);

  /**
   * @brief Consume characters while a predicate matches, writing each consumed character to a stream.
   * @param os Stream to write consumed characters to.
   * @param pred Called with each upcoming character; consumption stops on the first false result.
   */
  void eat_while(std::ostream& os, const std::function<bool(int)>& pred);

  /** @brief Consume the remainder of the current line, including its terminator. */
  void eat_line();

  /** @brief Check whether the stream is at end-of-file. @return True if at EOF. */
  [[nodiscard]] bool is_eof() const;

  /** @brief Read and consume the next character, updating line/column tracking. @return The character read, or EOF. */
  int get_char();

  /**
   * @brief Consume `n` characters (or until EOF, if sooner).
   * @param n Number of characters to consume.
   */
  void advance(unsigned n = 1);

  /** @brief Look at the next character without consuming it. @return The next character, or EOF. */
  [[nodiscard]] int peek_char() const;
};
