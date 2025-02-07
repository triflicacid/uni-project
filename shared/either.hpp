#pragma once

#include <optional>
#include <ostream>

template<typename U, typename V>
class Either {
  std::optional<U> left_;
  std::optional<V> right_;

public:
  Either(U value) : left_(std::move(value)) {};
  Either(V value) : right_(std::move(value)) {};

  bool is_left() const { return left_.has_value(); }

  const U& left() const { return left_.value(); }

  U take_left() { return std::move(left_.value()); }

  bool is_right() const { return right_.has_value(); }

  const V& right() const { return right_.value(); }

  V take_right() { return std::move(right_.value()); }

  void set(U value) {
    left_ = std::move(value);
    right_ = std::nullopt;
  }

  void set(V value) {
    right_ = std::move(value);
    left_ = std::nullopt;
  }

  friend std::ostream& operator<<(std::ostream& os, const Either<U, V>& e) {
    if (e.is_left()) {
      os << e.left();
    } else if (e.is_right()) {
      os << e.right();
    }
    return os;
  }
};
