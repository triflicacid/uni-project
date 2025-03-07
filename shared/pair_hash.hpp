#pragma once
#include <unordered_map>
#include <functional>
#include <string>
#include <utility>
#include "util.hpp"

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1,T2> &p) const {
    size_t h1 = std::hash<T1>{}(p.first);
    size_t h2 = std::hash<T2>{}(p.second);
    return hash_combine(h1, h2);
  }
};
