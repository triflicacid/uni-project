#ifndef SCROLLER_H
#define SCROLLER_H

#include <ftxui/component/component.hpp>

#include "ftxui/component/component_base.hpp"  // for Component

namespace ftxui {
/**
 * @brief Wrap a component in a vertically scrollable, focusable frame.
 * @param child Component to wrap.
 * @return The scrollable component.
 */
Component Scroller(Component child);

/**
 * @brief Wrap a component in a vertically scrollable, focusable frame and expose its selected line index.
 * @param child Component to wrap.
 * @param selected Set to point at the scroller's internal selected-line index.
 * @return The scrollable component.
 */
Component Scroller(Component child, int*& selected);
}
#endif /* end of include guard: SCROLLER_H */

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
