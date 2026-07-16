#pragma once

#include "tab.hpp"
#include "sources.hpp"

namespace visualiser::tabs {
  /**
   * @brief Tab showing the reconstructed, assembly, and Edel source panes side by side, synchronised to the CPU's current `$pc`, with stepping/breakpoint controls and a debug message log.
   */
  class CodeExecutionTab : public Tab {
  public:
    /** @brief Construct the tab with its display title. */
    CodeExecutionTab() : Tab("Execution") {}

  private:
    /** @brief Build the tab's panes, controls, and event handling. */
    void init() override;
  };
}// namespace visualiser::tabs
