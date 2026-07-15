#include "scroller.hpp"

#include <algorithm>                           // for max, min
#include <ftxui/component/component_base.hpp>  // for Component, ComponentBase
#include <ftxui/component/event.hpp>  // for Event, Event::ArrowDown, Event::ArrowUp, Event::End, Event::Home, Event::PageDown, Event::PageUp
#include <memory>   // for shared_ptr, allocator, __shared_ptr_access
#include <utility>  // for move

#include "ftxui/component/component.hpp"  // for Make
#include "ftxui/component/mouse.hpp"  // for Mouse, Mouse::WheelDown, Mouse::WheelUp
#include "ftxui/dom/deprecated.hpp"  // for text
#include "ftxui/dom/elements.hpp"  // for operator|, Element, size, vbox, EQUAL, HEIGHT, dbox, reflect, focus, inverted, nothing, select, vscroll_indicator, yflex, yframe
#include "ftxui/dom/node.hpp"      // for Node
#include "ftxui/dom/requirement.hpp"  // for Requirement
#include "ftxui/screen/box.hpp"       // for Box

namespace ftxui {

/**
 * @brief Component that renders a child inside a vertically scrollable, focusable frame with a visible cursor line.
 */
class ScrollerBase : public ComponentBase {
 public:
  /**
   * @brief Construct a scroller wrapping a child component.
   * @param child Component to wrap.
   */
  ScrollerBase(Component child) { Add(child); }

  /**
   * @brief Construct a scroller wrapping a child component and expose the selected line index.
   * @param child Component to wrap.
   * @param selected_ptr Set to point at the scroller's internal selected-line index.
   */
  ScrollerBase(Component child, int*& selected_ptr) {
    Add(child);
    selected_ptr = &selected_;
  }

 private:
  /**
   * @brief Render the child content overlaid with the current selection highlight, inside a scrollable frame.
   * @return The rendered element.
   */
  Element Render() final {
    auto focused = Focused() ? focus : ftxui::select;
    auto style = Focused() ? inverted : nothing;

    Element background = ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;
    return dbox({
               std::move(background),
               vbox({
                   text(L"") | size(HEIGHT, EQUAL, selected_),
                   text(L"") | style | focused,
               }),
           }) |
           vscroll_indicator | yframe | yflex | reflect(box_);
  }

  /**
   * @brief Handle keyboard/mouse navigation events, moving the selected line and clamping it to bounds.
   * @param event Event to handle.
   * @return True if the event changed the selected line.
   */
  bool OnEvent(Event event) final {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
      TakeFocus();

    int selected_old = selected_;
    if (event == Event::ArrowUp || event == Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
      selected_--;
    }
    if ((event == Event::ArrowDown || event == Event::Character('j') ||
         (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
      selected_++;
    }
    if (event == Event::PageDown)
      selected_ += box_.y_max - box_.y_min;
    if (event == Event::PageUp)
      selected_ -= box_.y_max - box_.y_min;
    if (event == Event::Home)
      selected_ = 0;
    if (event == Event::End)
      selected_ = size_;

    selected_ = std::max(0, std::min(size_ - 1, selected_));
    return selected_old != selected_;
  }

  /**
   * @brief Report that the scroller can take keyboard focus.
   * @return Always true.
   */
  bool Focusable() const final { return true; }

  int selected_ = 0;
  int size_ = 0;
  Box box_;
};

Component Scroller(Component child) {
  return Make<ScrollerBase>(std::move(child));
}

Component Scroller(Component child, int*& selected) {
  return Make<ScrollerBase>(std::move(child), selected);
}
}  // namespace ftxui

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
