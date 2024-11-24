#include "style.hpp"

using namespace ftxui;

Decorator visualiser::style::highlight_execution = bgcolor(Color::Yellow) | color(Color::Black);
Decorator visualiser::style::highlight_selected = bgcolor(Color::Cyan) | color(Color::Black);
Decorator visualiser::style::highlight_traced = bgcolor(Color::LightCyan3) | color(Color::Black);
Color visualiser::style::reg = Color::RedLight;
Color visualiser::style::value = Color::Violet;
