#pragma once

#include <ftxui/component/component.hpp>

namespace ftxui {
  Component Either(Component, Component, const bool *show);

  Component Either(Component, Component, std::function<bool()>);
}
