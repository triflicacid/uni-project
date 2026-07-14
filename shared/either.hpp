#pragma once

#include <optional>
#include <ostream>

/**
 * @brief A value holding exactly one of two types at a time, tagged by which one is currently held.
 *
 * Represents a discriminated union of `U` and `V`: at any point exactly one side is set (or, only
 * right after default-construction of `std::optional`, neither), and querying the wrong side is undefined.
 * @tparam U Type held on the "left" side.
 * @tparam V Type held on the "right" side.
 */
template<typename U, typename V>
class Either {
  std::optional<U> left_;
  std::optional<V> right_;

public:
  /**
   * @brief Construct holding a left value.
   * @param value Value to store on the left side.
   */
  Either(U value) : left_(std::move(value)) {};
  /**
   * @brief Construct holding a right value.
   * @param value Value to store on the right side.
   */
  Either(V value) : right_(std::move(value)) {};

  /** @brief Check whether the left side is currently held. @return True if the left value is set. */
  bool is_left() const { return left_.has_value(); }

  /** @brief Get the left value. @return Reference to the held left value. */
  const U& left() const { return left_.value(); }

  /** @brief Move the left value out. @return The held left value, moved from. */
  U take_left() { return std::move(left_.value()); }

  /** @brief Check whether the right side is currently held. @return True if the right value is set. */
  bool is_right() const { return right_.has_value(); }

  /** @brief Get the right value. @return Reference to the held right value. */
  const V& right() const { return right_.value(); }

  /** @brief Move the right value out. @return The held right value, moved from. */
  V take_right() { return std::move(right_.value()); }

  /**
   * @brief Set the left value, clearing the right side.
   * @param value New left value.
   */
  void set(U value) {
    left_ = std::move(value);
    right_ = std::nullopt;
  }

  /**
   * @brief Set the right value, clearing the left side.
   * @param value New right value.
   */
  void set(V value) {
    right_ = std::move(value);
    left_ = std::nullopt;
  }

  /**
   * @brief Write whichever side is currently held to a stream.
   * @param os Stream to write to.
   * @param e Value to write.
   * @return `os`, for chaining.
   */
  friend std::ostream& operator<<(std::ostream& os, const Either<U, V>& e) {
    if (e.is_left()) {
      os << e.left();
    } else if (e.is_right()) {
      os << e.right();
    }
    return os;
  }
};
