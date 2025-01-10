#pragma once

#include "lexer/Token.hpp"

namespace language::types {
  enum class Category {
    None,
    Integer,
    Float,
    User,
    Function,
  };

  class Type {
    public:
    [[nodiscard]] virtual size_t size() const { return 0; }

    [[nodiscard]] virtual Category category() const { return Category::None; }

    [[nodiscard]] virtual std::string repr() const { return ""; }

    virtual void debug_print(std::ostream &stream, const std::string &prefix) const {};
  };

  /** return if type `fst` can be implicitly cast to `snd` (without additional code) */
  bool can_implicitly_cast_to(const Type *fst, const Type *snd);
}
