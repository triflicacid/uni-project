#pragma once

#include <ftxui/component/component.hpp>

namespace visualiser {
  /**
   * @brief Create a dropdown that shows a scrollable radiobox of entries when open, adapted from FTXUI's `dropdown_custom` example.
   * @param entries List of entries to display in the dropdown.
   * @param selected Index of the currently selected entry.
   * @return The constructed dropdown component.
   */
  ftxui::Component CustomDropdown(ftxui::ConstStringListRef entries, int *selected);

  /**
   * @brief Create a dropdown that shows a scrollable radiobox of entries when open, using caller-supplied options.
   * @param entries List of entries to display in the dropdown.
   * @param selected Index of the currently selected entry.
   * @param options Base dropdown options; `radiobox` and `transform` fields are overwritten.
   * @return The constructed dropdown component.
   */
  ftxui::Component CustomDropdown(ftxui::ConstStringListRef entries, int *selected, ftxui::DropdownOption options);
}
