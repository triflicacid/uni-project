#include "custom_dropdown.hpp"

#include <utility>

ftxui::Component visualiser::CustomDropdown(ftxui::ConstStringListRef entries, int *selected) {
  return CustomDropdown(std::move(entries), selected, {});
}

ftxui::Component visualiser::CustomDropdown(ftxui::ConstStringListRef entries, int *selected, ftxui::DropdownOption options) {
  using namespace ftxui;
  options.radiobox.entries = entries;
  options.radiobox.selected = selected;
  options.transform = [](bool open, Element checkbox, Element radiobox) {
                        if (open) {
                          return vbox({
                                          checkbox | inverted,
                                          radiobox | vscroll_indicator | frame |
                                          size(HEIGHT, LESS_THAN, 10),
                                          filler(),
                                      });
                        }
                        return vbox({
                                        checkbox,
                                        filler(),
                                    });
                      };
  return Dropdown(options);
}
