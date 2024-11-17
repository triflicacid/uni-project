#pragma once

#include <memory>
#include <ftxui/component/component.hpp>
#include <map>

namespace visualiser::tabs {
    using EventMap = std::map<ftxui::Event, std::function<void(ftxui::Event &)>>;
}
