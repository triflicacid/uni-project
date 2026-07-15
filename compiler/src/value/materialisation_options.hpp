#pragma once

#include "memory/storage_location.hpp"

namespace lang::value {
  /**
   * @brief Parameters controlling Value::materialise(): an optional destination location, copy-vs-move semantics, and a source origin for line tracking.
   */
  struct MaterialisationOptions {
    optional_ref<const memory::StorageLocation> target = std::nullopt; ///< Destination location to copy/move the materialised value into, if any.
    bool copy_or_move = false; ///< False for move, true for copy.
    std::optional<Location> origin = std::nullopt; ///< Source location, for line-origin tracking.
  };
}
