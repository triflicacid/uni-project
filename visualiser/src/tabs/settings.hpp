#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  class SettingsTab : public Tab {
  public:
    SettingsTab() : Tab("Settings") {}

  private:
    void init() override;
  };
}// namespace visualiser::tabs
