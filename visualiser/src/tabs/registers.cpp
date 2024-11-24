#include "registers.hpp"
#include "constants.hpp"
#include "processor.hpp"
#include "style.hpp"
#include "util.hpp"

enum class Field {
  Hex,
  Int,
  Long,
  Float,
  Double
};

namespace state {
  std::vector<std::string> reg_names;
  int current_reg = 0;

  namespace inputs {
    std::string s_hex, s_int, s_long, s_float, s_double;
    ftxui::Component i_hex, i_int, i_long, i_float, i_double;
  }// namespace inputs
}// namespace state

static ftxui::MenuEntryOption format_register_item() {
  using namespace ftxui;
  return {
      .transform = [&](const EntryState &st) {
        size_t idx = st.label.find('=');
        auto el = hbox({text(st.label.substr(0, idx)),
                        text(" = "),
                        text(st.label.substr(idx)) | color(visualiser::style::reg)});
        if (st.active) el |= visualiser::style::highlight_traced;
        return el;
      }};
}

static void sync_inputs() {
  auto reg = static_cast<constants::registers::reg>(state::current_reg);
  uint64_t val = visualiser::processor::cpu.reg(reg, true);

  state::inputs::s_hex = to_hex_string(val);
  state::inputs::s_int = std::to_string(*(int *) &val);
  state::inputs::s_long = std::to_string(val);
  state::inputs::s_float = std::to_string(*(float *) &val);
  state::inputs::s_double = std::to_string(*(double *) &val);
}

static void update_reg_from_input(Field update) {
  auto reg = static_cast<constants::registers::reg>(state::current_reg);

  switch (update) {
    case Field::Hex: {
      try {
        uint64_t value = std::stoull(state::inputs::s_hex, nullptr, 16);
        visualiser::processor::cpu.reg_set(reg, value, true);
      } catch (std::exception &e) {}
      break;
    }
    case Field::Int: {
      try {
        int value = std::stoi(state::inputs::s_int);
        uint64_t buffer = *(uint32_t *) &value;
        visualiser::processor::cpu.reg_set(reg, buffer, true);
      } catch (std::exception &e) {}
      break;
    }
    case Field::Long: {
      try {
        long value = std::stoll(state::inputs::s_long);
        uint64_t buffer = *(uint64_t *) &value;
        visualiser::processor::cpu.reg_set(reg, buffer, true);
      } catch (std::exception &e) {}
      break;
    }
    case Field::Float: {
      try {
        float value = std::stof(state::inputs::s_float);
        uint64_t buffer = *(uint32_t *) &value;
        visualiser::processor::cpu.reg_set(reg, buffer, true);
      } catch (std::exception &e) {}
      break;
    }
    case Field::Double: {
      try {
        double value = std::stod(state::inputs::s_double);
        uint64_t buffer = *(uint64_t *) &value;
        visualiser::processor::cpu.reg_set(reg, buffer, true);
      } catch (std::exception &e) {}
      break;
    }
  }

  sync_inputs();
}

static void init_inputs() {
  using namespace state::inputs;

  i_hex = ftxui::Input({
                           .content = &s_hex,
                           .multiline = false,
                           .on_enter = [] { update_reg_from_input(Field::Hex); },
                       });
  i_int = ftxui::Input({.content = &s_int,
                           .multiline = false,
                           .on_enter = [] { update_reg_from_input(Field::Int); }});
  i_long = ftxui::Input({.content = &s_long,
                            .multiline = false,
                            .on_enter = [] { update_reg_from_input(Field::Long); }});
  i_float = ftxui::Input({.content = &s_float,
                             .multiline = false,
                             .on_enter = [] { update_reg_from_input(Field::Float); }});
  i_double = ftxui::Input({.content = &s_double,
                              .multiline = false,
                              .on_enter = [] { update_reg_from_input(Field::Double); }});
}

void visualiser::tabs::RegistersTab::init() {
  using namespace ftxui;

  for (int r = 0; r < constants::registers::count; r++) {
    state::reg_names.push_back("$" + constants::registers::to_string(static_cast<constants::registers::reg>(r)));
  }

  init_inputs();
  sync_inputs();

  auto register_list = Renderer(Menu(&state::reg_names, &state::current_reg),
                                [&] {
                                  std::vector<Element> left, right;

                                  for (int r = 0; r < constants::registers::count; r++) {
                                    auto name = text("$" + constants::registers::to_string(
                                        static_cast<constants::registers::reg>(r)));
                                    if (r == state::current_reg) name |= style::highlight_traced;
                                    left.push_back(name);

                                    auto value = hbox({text(" = "),
                                                       text("0x" + to_hex_string(visualiser::processor::cpu.reg(
                                                           static_cast<constants::registers::reg>(r), true))) |
                                                       color(style::reg)});
                                    if (r == state::current_reg) value |= style::highlight_traced;
                                    right.push_back(value);
                                  }

                                  return hbox({vbox(left), vbox(right)});
                                });

  auto register_view = Renderer(
      Container::Vertical({state::inputs::i_hex, state::inputs::i_int, state::inputs::i_long, state::inputs::i_float,
                           state::inputs::i_double}),
      [&] {
        auto reg = static_cast<constants::registers::reg>(state::current_reg);

        return vbox({
                        center(text("Register $" + constants::registers::to_string(reg)) | bold | border),
                        hbox({text("Value = "),
                              text("0x") | color(style::value),
                              state::inputs::i_hex->Render() | color(style::value)}),
                        hbox({text("Integer = "),
                              state::inputs::i_int->Render() | color(style::value)}),
                        hbox({text("Long = "),
                              state::inputs::i_long->Render() | color(style::value)}),
                        hbox({text("Float = "),
                              state::inputs::i_float->Render() | color(style::value)}),
                        hbox({text("Double = "),
                              state::inputs::i_double->Render() | color(style::value)}),
                    }) |
               border;
      });


  content_ = Container::Horizontal({
                                       register_list,
                                       register_view,
                                   });
}
