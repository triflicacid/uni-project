#include "settings.hpp"
#include "processor/src/debug.hpp"
#include "components/checkbox.hpp"

namespace state {
  namespace debug {
    static ftxui::Component input_list;
    static bool all_checked = false;
  }
}// namespace state

void visualiser::tabs::SettingsTab::init() {
  using namespace ftxui;

  state::debug::all_checked = processor::debug::all();

  Components debug_checkboxes{
    create_checkbox("- All -", [] {
      processor::debug::set_all(!processor::debug::all());
      state::debug::all_checked = processor::debug::all();
    }, state::debug::all_checked),
    create_checkbox("CPU", processor::debug::cpu),
    create_checkbox("Instruction Operand Resolution", processor::debug::args),
    create_checkbox("Memory Access", processor::debug::mem),
    create_checkbox("Register Access", processor::debug::reg),
    create_checkbox("Zero Flag Access", processor::debug::zflag),
    create_checkbox("Instruction Conditional Guard Resolution", processor::debug::conditionals),
    create_checkbox("Error Details", processor::debug::errs),
  };
  state::debug::input_list = Container::Vertical(std::move(debug_checkboxes));

  Component layout = state::debug::input_list;

  content_ = Renderer(layout, [] {
    return vbox({
      window(
          text("Processor Debug Messages"),
          state::debug::input_list->Render(),
          LIGHT
          ),
    });
  });
}
