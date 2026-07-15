#pragma once

#include "memory/storage_location.hpp"

namespace lang::value {
  /**
   * @brief Parameters controlling Value::materialise(): an optional destination location, copy-vs-move semantics, and a source origin for line tracking.
   */
  struct MaterialisationOptions {
    optional_ref<const memory::StorageLocation> target = std::nullopt;
    bool copy_or_move = false; // false for move, true for copy
    std::optional<Location> origin = std::nullopt;
  };
}
