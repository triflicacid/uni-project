#pragma once

#include <ftxui/component/component.hpp>
#include <map>
#include "tabs.hpp"

namespace visualiser::tabs::execution {
    extern std::string title;
    extern int index;
    extern ftxui::Component content;

    void init();

    bool on_event(ftxui::Event &);
}
