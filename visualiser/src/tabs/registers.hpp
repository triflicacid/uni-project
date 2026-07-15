#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  /**
   * @brief Tab showing a selectable list of CPU registers alongside an editor for the selected register's value in multiple formats, with dedicated controls for the `$flag` register's bit fields.
   */
  class RegistersTab : public Tab {
  public:
    /** @brief Construct the tab with its display title. */
    RegistersTab() : Tab("Registers") {}

  private:
    /** @brief Build the register list, value editor, and `$flag` controls. */
    void init() override;
  };
}// namespace visualiser::tabs
