#pragma once

#include <ftxui/component/component.hpp>
#include <utility>
#include <map>

namespace visualiser::tabs {
  class Tab {
  protected:
    std::string title_;
    ftxui::Component content_;
    ftxui::Component help_;

    virtual void init() = 0;

  public:
    explicit Tab(std::string title) : title_(std::move(title)), content_(nullptr) {}

    [[nodiscard]] const std::string &title() const { return title_; }

    ftxui::Component content() {
      if (content_) return content_;
      init();
      return content_;
    }

    ftxui::Component help() {
      if (help_) return help_;
      init();
      return help_;
    }
  };

  // given map of key-descriptions, return help pane
  ftxui::Element create_key_help_pane(const std::map<std::string, std::string> &keys);

}// namespace visualiser::tabs
