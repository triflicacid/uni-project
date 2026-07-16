#pragma once

#include <string>
#include <ftxui/component/component.hpp>

/**
 * @brief Create a checkbox that toggles a boolean when activated.
 * @param label Text displayed next to the checkbox.
 * @param control Boolean bound to the checkbox's checked state.
 * @return The constructed checkbox component.
 */
ftxui::Component create_checkbox(std::string label, bool& control);

/**
 * @brief Create a checkbox that toggles a boolean and invokes a callback on change.
 * @param label Text displayed next to the checkbox.
 * @param cb Callback invoked whenever the checkbox state changes.
 * @param control Boolean bound to the checkbox's checked state.
 * @return The constructed checkbox component.
 */
ftxui::Component create_checkbox(std::string label, std::function<void()> cb, bool& control);
