#pragma once

#include "tab.hpp"
#include <ftxui/component/component.hpp>

namespace visualiser::tabs {
  class RegistersTab : public Tab {
  public:
    RegistersTab() : Tab("Registers") {}

  private:
    void init() override;
  };
}// namespace visualiser::tabs
