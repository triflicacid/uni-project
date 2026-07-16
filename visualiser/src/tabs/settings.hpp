#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  /**
   * @brief Tab exposing checkboxes to toggle which categories of processor debug messages are emitted.
   */
  class SettingsTab : public Tab {
  public:
    /** @brief Construct the tab with its display title. */
    SettingsTab() : Tab("Settings") {}

  private:
    /** @brief Build the debug-message category checkboxes. */
    void init() override;
  };
}// namespace visualiser::tabs
