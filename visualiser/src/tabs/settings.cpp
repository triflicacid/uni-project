#include "settings.hpp"
#include "processor/src/debug.hpp"
#include "components/checkbox.hpp"

namespace state {
  namespace debug {
    static ftxui::Component input_list;
  }
}// namespace state

void visualiser::tabs::SettingsTab::init() {
  using namespace ftxui;

  state::debug::input_list = Container::Vertical({
    create_checkbox("CPU", processor::debug::cpu),
    create_checkbox("Instruction Operand Resolution", processor::debug::args),
    create_checkbox("Memory Access", processor::debug::mem),
    create_checkbox("Register Access", processor::debug::reg),
    create_checkbox("Zero Flag Access", processor::debug::zflag),
    create_checkbox("Instruction Conditional Guard Resolution", processor::debug::conditionals),
    create_checkbox("Error Details", processor::debug::errs),
  });

  Component layout = state::debug::input_list; //Container::Vertical({debug_list});

  content_ = Renderer(layout, [] {
    return vbox({
      window(
          text("Debug Flags"),
          state::debug::input_list->Render(),
          LIGHT
          ),
    });
  });
}
