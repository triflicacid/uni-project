#pragma once
#include <unordered_map>
#include <functional>
#include <string>
#include <utility>
#include "util.hpp"

/** @brief A hash functor for `std::pair`, for use as the `Hash` template argument of `unordered_map`/`unordered_set`. */
struct pair_hash {
  /**
   * @brief Hash a pair by combining the hashes of its two elements.
   * @tparam T1 Type of the pair's first element.
   * @tparam T2 Type of the pair's second element.
   * @param p Pair to hash.
   * @return Combined hash value.
   */
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1,T2> &p) const {
    size_t h1 = std::hash<T1>{}(p.first);
    size_t h2 = std::hash<T2>{}(p.second);
    return hash_combine(h1, h2);
  }
};
