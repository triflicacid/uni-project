#pragma once

#include <ftxui/component/component.hpp>

namespace ftxui {
  Element boolean(bool b);

  Component Boolean(const bool *b);

  Component Boolean(std::function<bool()>);
}
