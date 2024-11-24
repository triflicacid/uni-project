#pragma once

#include <ftxui/component/component.hpp>

namespace visualiser::style {
  extern ftxui::Decorator highlight_execution; // highlight used for current line we're executing
  extern ftxui::Decorator highlight_selected; // highlight used for line we've selected
  extern ftxui::Decorator highlight_traced; // highlight used for traced lines
  extern ftxui::Color reg;
  extern ftxui::Color value;
}
