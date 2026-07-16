#pragma once

#include <optional>

/**
 * @brief An optional reference to a `T`: either a reference to an existing value, or empty.
 * @tparam T Type referenced.
 */
template<typename T>
using optional_ref = std::optional<std::reference_wrapper<T>>;
