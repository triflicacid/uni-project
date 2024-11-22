#pragma once

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include "named_fstream.hpp"

namespace visualiser {
    extern ftxui::Component main;
    extern ftxui::ScreenInteractive screen;

    void init();

    void display();
}
