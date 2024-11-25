#pragma once

#include <ftxui/component/component.hpp>

namespace visualiser::style {
  extern ftxui::Decorator highlight_execution; // highlight used for current line we're executing
  extern ftxui::Decorator highlight_selected; // highlight used for line we've selected
  extern ftxui::Decorator highlight_traced; // highlight used for traced lines
  extern ftxui::Decorator reg; // style for a register value
  extern ftxui::Decorator value; // style for a value (e.g., number)
  extern ftxui::Decorator ok; // style for a status:ok string
  extern ftxui::Decorator error; // style for a status:error string
}
