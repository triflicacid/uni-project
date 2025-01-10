#pragma once

#include "tab.hpp"

namespace visualiser::tabs {
  class SourcesTab : public Tab {
  public:
    SourcesTab() : Tab("Sources") {}

  private:
    void init() override;
  };
}// namespace visualiser::tabs
