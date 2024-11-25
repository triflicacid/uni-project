#include "style.hpp"

using namespace ftxui;

Decorator visualiser::style::highlight_execution = bgcolor(Color::Yellow) | color(Color::Black);
Decorator visualiser::style::highlight_selected = bgcolor(Color::Cyan) | color(Color::Black);
Decorator visualiser::style::highlight_traced = bgcolor(Color::LightCyan3) | color(Color::Black);
Decorator visualiser::style::reg = color(Color::RedLight);
Decorator visualiser::style::ok = color(Color::GreenLight);
Decorator visualiser::style::error = color(Color::Red);
Decorator visualiser::style::value = color(Color::Violet);
