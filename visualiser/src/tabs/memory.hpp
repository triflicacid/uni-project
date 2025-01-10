#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  class MemoryTab : public Tab {
  public:
    MemoryTab() : Tab("Memory") {}

  private:
    void init() override;
  };
}// namespace visualiser::tabs
