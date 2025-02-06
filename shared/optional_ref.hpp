#pragma once

#include <optional>

template<typename T>
using optional_ref = std::optional<std::reference_wrapper<T>>;
