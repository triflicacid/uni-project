#pragma once

#include <set>
#include "either.hpp"
#include "token.hpp"
#include "istream_wrapper.hpp"
#include "messages/MessageWithSource.hpp"

namespace lang::lexer {
  /** @brief Tokenizer that reads a source stream one token at a time, turning raw characters into `Token`s for the `parser::Parser` to consume. */
  class Lexer {
    IStreamWrapper& stream_;

    /**
     * @brief Build a token located at the stream's current position.
     * @param image Source text that produced the token.
     * @param type Token type.
     * @return Newly constructed token.
     */
    Token token(const std::string& image, TokenType type) const;

  public:
    /**
     * @brief Construct a lexer over the given source stream.
     * @param stream Stream to read characters from.
     */
    explicit Lexer(IStreamWrapper& stream) : stream_(stream) {}

    /** @brief Return the underlying source stream. */
    IStreamWrapper& stream() const { return stream_; }

    /**
     * @brief Get the raw source text of a given line.
     * @param line 1-indexed line number.
     * @return The line's text.
     */
    std::string get_line(unsigned int line) const { return stream_.get_line(line); }

    /** @brief Return the name of the source being lexed (e.g. filename), or "<file>" if unnamed. */
    std::string get_source_name() const { return stream_.get_name("<file>"); }

    /** @brief Test whether the underlying stream has been fully consumed. */
    bool is_eof() const { return stream_.is_eof(); }

    /**
     * @brief Read and extract the next token from the input stream, skipping whitespace and comments.
     * @return The next lexed token.
     */
    Token next();
  };
}
