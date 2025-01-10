#include "checkbox.hpp"

ftxui::Component create_checkbox(std::string label, bool& control) {
  return ftxui::Checkbox({
                           .label = label,
                           .checked = &control,
  });
}

ftxui::Component create_checkbox(std::string label, std::function<void()> cb, bool& control) {
  return ftxui::Checkbox({
                             .label = label,
                             .checked = &control,
                             .on_change = cb
                         });
}
