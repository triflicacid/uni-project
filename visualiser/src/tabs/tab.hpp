#pragma once

#include <ftxui/component/component.hpp>
#include <utility>
#include <map>

/** @brief The visualiser's top-level UI tabs (execution, memory, registers, sources, settings) and their shared Tab base class. */
namespace visualiser::tabs {
  /**
   * @brief Base class for a top-level UI tab, lazily building its content and optional help pane on first access.
   */
  class Tab {
  protected:
    std::string title_; ///< Text shown in the tab navigation bar.
    bool called_init_ = false; ///< Whether @ref init has already run.
    ftxui::Component content_; ///< Main tab content; cannot be null.
    ftxui::Component help_; ///< Help pane; may be null if the tab has no help content.

    /** @brief Build `content_` (and optionally `help_`); called once on first access to either. */
    virtual void init() = 0;

    /** @brief Run `init()` exactly once, on the first call. */
    void init_() {
      if (!called_init_) {
        init();
        called_init_ = true;
      }
    }

  public:
    /**
     * @brief Construct a tab with the given display title.
     * @param title Text shown in the tab navigation bar.
     */
    explicit Tab(std::string title) : title_(std::move(title)), content_(nullptr) {}

    /**
     * @brief Get the tab's display title.
     * @return The tab's title.
     */
    [[nodiscard]] const std::string &title() const { return title_; }

    /**
     * @brief Get the tab's main content component, initialising it on first call.
     * @return The tab's content component.
     */
    ftxui::Component content() {
      init_();
      return content_;
    }

    /**
     * @brief Get the tab's help pane component, initialising it on first call.
     * @return The tab's help component, or null if the tab has no help pane.
     */
    ftxui::Component help() {
      init_();
      return help_;
    }
  };

  /**
   * @brief Build a help pane element listing key bindings and their descriptions.
   * @param keys Map from key label to description.
   * @return The rendered help pane element.
   */
  ftxui::Element create_key_help_pane(const std::map<std::string, std::string> &keys);

}// namespace visualiser::tabs
