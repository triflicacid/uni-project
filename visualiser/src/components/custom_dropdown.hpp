#pragma once

#include <ftxui/component/component.hpp>

namespace visualiser {
  // create custom Dropdown()
  // courtesy of `examples/component/dropdown_custom.cpp`
  ftxui::Component CustomDropdown(ftxui::ConstStringListRef entries, int *selected);

  ftxui::Component CustomDropdown(ftxui::ConstStringListRef entries, int *selected, ftxui::DropdownOption options);
}
