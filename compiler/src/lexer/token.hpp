#pragma once

#include "location.hpp"
#include "token_type.hpp"
#include "messages/MessageWithSource.hpp"
#include "istream_wrapper.hpp"
#include <deque>
#include <set>

namespace lang::lexer {
  class Lexer;

  /**
   * @brief Plain value pairing a `TokenType` with the source text that produced it.
   *
   * Equality treats an empty `image` on either side as a wildcard match, so a
   * `BasicToken` built from just a type can compare equal to any concrete token
   * of that type; this is what lets expected-token sets be built without images.
   */
  struct BasicToken {
    TokenType type; ///< Type of the token.
    std::string image; ///< Source text that produced this token.

    /**
     * @brief Construct a token with no source image.
     * @param type Token type.
     */
    explicit BasicToken(TokenType type) : type(type) {}

    /**
     * @brief Construct a token with the given type and source image.
     * @param type Token type.
     * @param image Source text that produced this token.
     */
    BasicToken(TokenType type, std::string image) : type(type), image(std::move(image)) {}

    /** @brief Return the length, in characters, of the source image. @return The image length. */
    size_t length() const { return image.size(); }

    /** @brief Test whether this token marks end-of-file. @return True if this token is the end-of-file token. */
    bool is_eof() const;

    /** @brief Test whether this token is not the `invalid` sentinel type. @return True if this token is valid. */
    bool is_valid() const;

    /** @brief Render this token as a human-readable string, including its image if distinct from the type name. @return The rendered string. */
    std::string to_string() const;

    /**
     * @brief Order tokens by type then by image, needed to store them in a `std::set`.
     * @param other Token to compare against.
     * @return True if this token sorts before `other`.
     */
    bool operator<(const BasicToken& other) const {
      return type == other.type
        ? image < other.image
        : type < other.type;
    }

    /**
     * @brief Compare tokens by type, treating an empty image on either side as a wildcard.
     * @param other Token to compare against.
     * @return True if the tokens are considered equal.
     */
    bool operator==(const BasicToken& other) const {
      return type == other.type && (image.empty() || other.image.empty() || image == other.image);
    }
  };

  /** @brief Set of distinct `BasicToken`s, used to describe expected tokens (type + image) at a parse point. */
  using TokenSet = std::set<BasicToken>;

  /**
   * @brief Union a collection of token sets into one set.
   * @param sets Sets to merge.
   * @return Union of all the given sets.
   */
  TokenSet merge_sets(const std::vector<TokenSet>& sets);

  /**
   * @brief Convert a set of token types into a set of imageless `BasicToken`s.
   * @param types Token types to convert.
   * @return Equivalent set of `BasicToken`s, each with an empty image.
   */
  TokenSet convert_set(const TokenTypeSet& types);

  /** @brief A token as produced by the lexer: a `BasicToken` plus its originating stream and source location, able to generate diagnostic messages about itself. */
  struct Token : BasicToken, message::MessageGenerator {
    std::reference_wrapper<IStreamWrapper> origin; ///< Stream this token was lexed from.
    Location loc; ///< Source location this token was created at.

    /**
     * @brief Construct a token with full source provenance.
     * @param type Token type.
     * @param image Source text that produced this token.
     * @param origin Stream the token was lexed from.
     * @param loc Source location of the token.
     */
    Token(TokenType type, std::string image, IStreamWrapper& origin, Location loc) : BasicToken(type, std::move(image)), origin(origin), loc(loc) {}

    /**
     * @brief Generate a diagnostic message pointing at this token's source location.
     * @param level Severity level of the message.
     * @return Newly constructed message.
     */
    std::unique_ptr<message::Message> generate_message(message::Level level) const override;

    /**
     * @brief Generate a syntax-error message reporting this token in place of an expected token type.
     * @param expected_types Set of token types that would have been valid here.
     * @return Newly constructed error message.
     */
    std::unique_ptr<message::Message> generate_syntax_error(const lexer::TokenTypeSet& expected_types) const;

    /**
     * @brief Generate a syntax-error message reporting this token in place of one or more expected tokens (type and image).
     * @param expected_tokens Set of tokens that would have been valid here.
     * @return Newly constructed error message.
     */
    std::unique_ptr<message::Message> generate_detailed_syntax_error(const lexer::TokenSet& expected_tokens) const;

    /**
     * @brief Create a placeholder invalid token with no meaningful location.
     * @param stream Stream to attribute the token to.
     * @return An invalid token; its location must not be used for lookups.
     */
    static Token invalid(IStreamWrapper& stream);
  };

  /** @brief Abstract mixin for anything spanning a start and end token, letting any source range generate a diagnostic message covering it. */
  struct TokenSpan : message::MessageGenerator {
    /** @brief Return the first token of this span. @return The start token. */
    virtual const Token& token_start() const = 0;

    /** @brief Return the last token of this span. @return The end token. */
    virtual const Token& token_end() const = 0;

    /**
     * @brief Generate a diagnostic message covering the full range from `token_start()` to `token_end()`.
     * @param level Severity level of the message.
     * @return Newly constructed message.
     */
    std::unique_ptr<message::Message> generate_message(message::Level level) const override;
  };
}
