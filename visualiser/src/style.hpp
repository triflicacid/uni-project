#pragma once

#include <ftxui/component/component.hpp>

/** @brief Shared FTXUI text decorators used to style the visualiser's UI elements consistently. */
namespace visualiser::style {
  extern ftxui::Decorator highlight; ///< General highlight.
  extern ftxui::Decorator highlight_execution; ///< Highlight used for the current line being executed.
  extern ftxui::Decorator highlight_selected; ///< Highlight used for the currently selected line.
  extern ftxui::Decorator highlight_traced; ///< Highlight used for traced lines.
  extern ftxui::Decorator reg; ///< Style for a register value.
  extern ftxui::Decorator value; ///< Style for a value (e.g. a number).
  extern ftxui::Decorator ok; ///< Style for a status:ok string.
  extern ftxui::Decorator bad; ///< Style for a string that's neither status:ok nor quite status:error.
  extern ftxui::Decorator error; ///< Style for a status:error string.
  extern ftxui::Decorator breakpoint_colour; ///< Colour used for the breakpoint marker.
  extern std::string breakpoint_icon; ///< Icon/text used for the breakpoint marker.

  /**
   * @brief Build the styled breakpoint marker element shown next to lines with a breakpoint set.
   * @return The rendered breakpoint prefix element.
   */
  ftxui::Element breakpoint_prefix();
}
