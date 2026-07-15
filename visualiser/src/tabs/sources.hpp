#pragma once

#include "tab.hpp"

namespace visualiser::tabs {
  /**
   * @brief Tab listing every known source/assembly/language file with a viewer pane, supporting line selection, breakpoint toggling, and tracing a line forward/backward through the lang/asm/source chain.
   */
  class SourcesTab : public Tab {
  public:
    /** @brief Construct the tab with its display title. */
    SourcesTab() : Tab("Sources") {}

  private:
    /** @brief Build the file menu, viewer pane, and event handling. */
    void init() override;
  };
}// namespace visualiser::tabs
