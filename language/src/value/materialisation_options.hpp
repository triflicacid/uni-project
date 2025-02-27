#pragma once

#include "memory/storage_location.hpp"

namespace lang::value {
  struct MaterialisationOptions {
    optional_ref<const memory::StorageLocation> target = std::nullopt;
    bool copy_or_move = false; // false for move, true for copy
  };
}
