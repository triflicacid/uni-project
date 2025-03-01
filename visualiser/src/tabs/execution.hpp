#pragma once

#include "tab.hpp"
#include "sources.hpp"

namespace visualiser::tabs {
  class CodeExecutionTab : public Tab {
  public:
    CodeExecutionTab() : Tab("Execution") {}

  private:
    void init() override;
  };
}// namespace visualiser::tabs
