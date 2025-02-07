#pragma once

#include "location.hpp"
#include "token_type.hpp"
#include "messages/MessageWithSource.hpp"
#include "istream_wrapper.hpp"
#include <deque>
#include <set>

namespace lang::lexer {
  class Lexer;

  struct BasicToken {
    TokenType type; // type of the token
    std::string image; // the image which created us (source)

    explicit BasicToken(TokenType type) : type(type) {}
    BasicToken(TokenType type, std::string image) : type(type), image(std::move(image)) {}

    size_t length() const { return image.size(); }
    bool is_eof() const;
    bool is_valid() const;

    std::string to_string() const;

    // required for storing in a set
    bool operator<(const BasicToken& other) const {
      return type == other.type
        ? image < other.image
        : type < other.type;
    }

    bool operator==(const BasicToken& other) const {
      return type == other.type && (image.empty() || image == other.image);
    }
  };

  using TokenSet = std::set<BasicToken>;

  TokenSet merge_sets(const std::vector<TokenSet>& sets);

  TokenSet convert_set(const TokenTypeSet& types);

  struct Token : BasicToken, message::MessageGenerator {
    std::reference_wrapper<IStreamWrapper> origin;
    Location loc; // the location in which we were created
    uint64_t value = 0; // integer value associated with this token, used for literals

    Token(TokenType type, std::string image, IStreamWrapper& origin, Location loc) : BasicToken(type, std::move(image)), origin(origin), loc(loc) {}

    // given numerical type TokenType (i.e., int8), parse image_ as a number of that type
    // return if parse was successful
    bool parse_numerical(TokenType type);

    // generate a message of the given type about this token
    std::unique_ptr<message::Message> generate_message(message::Level level) const override;

    // generate an error message about a syntax error with this token
    std::unique_ptr<message::Message> generate_syntax_error(const lexer::TokenTypeSet& expected_types) const;

    // generate an error message about a syntax error with this token
    std::unique_ptr<message::Message> generate_detailed_syntax_error(const lexer::TokenSet& expected_tokens) const;

    // create an invalid token
    // token's location is dud, so don't try to use it to look anything up
    static Token invalid(IStreamWrapper& stream);
  };

  struct TokenSpan : message::MessageGenerator {
    virtual const Token& token_start() const = 0;

    virtual const Token& token_end() const = 0;

    // generate a message of the given type about this token
    std::unique_ptr<message::Message> generate_message(message::Level level) const override;
  };
}
