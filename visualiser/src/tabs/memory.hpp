#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  /**
   * @brief Tab showing a paged hex grid of the CPU's memory, with a cursor for editing individual bytes/values and highlighting of `$pc`/`$sp`/`$fp`.
   */
  class MemoryTab : public Tab {
  public:
    /** @brief Construct the tab with its display title. */
    MemoryTab() : Tab("Memory") {}

  private:
    /** @brief Build the memory grid, address editor, and event handling. */
    void init() override;
  };
}// namespace visualiser::tabs
