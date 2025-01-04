#pragma once

#include <string>
#include <ftxui/component/component.hpp>

/** Create a checkbox which toggles the given Boolean. */
ftxui::Component create_checkbox(std::string label, bool& control);

/** Create a checkbox which calls the given function and is controlled by the Boolean. */
ftxui::Component create_checkbox(std::string label, std::function<void()> cb, bool& control);
