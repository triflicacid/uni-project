#pragma once

#include <ftxui/component/component.hpp>
#include <utility>
#include <map>

namespace visualiser::tabs {
  class Tab {
  protected:
    std::string title_;
    bool called_init_ = false;
    ftxui::Component content_; // ain tab content, cannot be null
    ftxui::Component help_; // help pane, may be null

    virtual void init() = 0;

    void init_() {
      if (!called_init_) {
        init();
        called_init_ = true;
      }
    }

  public:
    explicit Tab(std::string title) : title_(std::move(title)), content_(nullptr) {}

    [[nodiscard]] const std::string &title() const { return title_; }

    ftxui::Component content() {
      init_();
      return content_;
    }

    ftxui::Component help() {
      init_();
      return help_;
    }
  };

  // given map of key-descriptions, return help pane
  ftxui::Element create_key_help_pane(const std::map<std::string, std::string> &keys);

}// namespace visualiser::tabs
