#pragma once

#include <ftxui/component/component.hpp>
#include <utility>

namespace visualiser::tabs {
  class Tab {
  protected:
    std::string title_;
    ftxui::Component content_;

    virtual void init() = 0;

  public:
    explicit Tab(std::string title) : title_(std::move(title)), content_(nullptr) {}

    [[nodiscard]] const std::string &title() const { return title_; }

    ftxui::Component content() {
      if (content_) return content_;
      init();
      return content_;
    }
  };
}// namespace visualiser::tabs
