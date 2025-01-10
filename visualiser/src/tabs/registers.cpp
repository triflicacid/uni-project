#include "registers.hpp"
#include "constants.hpp"
#include "processor.hpp"
#include "style.hpp"
#include "util.hpp"
#include "components/checkbox.hpp"
#include "components/boolean.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/table.hpp>

enum class Field {
  Hex,
  Int,
  Long,
  Float,
  Double
};

namespace state {
  static std::vector<std::string> reg_names; // list of register names, used in menu
  static int current_reg = 0; // index of the current register
  static std::optional<uint64_t> copied_reg; // copied register contents

  namespace inputs {
    static std::string s_hex, s_int, s_long, s_float, s_double;
    static ftxui::Component i_hex, i_int, i_long, i_float, i_double;
  }// namespace inputs

  namespace flag { // fields for the $flag register
    static ftxui::Component is_zero_toggle, is_running_toggle, cmp_dropdown;
    static bool is_zero = false, is_running = false, in_interrupt = false;
    static int cmp_dropdown_select = 0;
    static std::vector<std::string> cmp_entries;
  }
}// namespace state

// read the given register
static uint64_t read(int i) {
  return visualiser::processor::cpu.reg($reg(i), true);
}

// read the current register
static uint64_t read() {
  return read(state::current_reg);
}

// write to the current register
static void write(uint64_t v) {
  visualiser::processor::cpu.reg_set($reg(state::current_reg), v, true);
}

// ensure all inputs read the correct values
static void sync_inputs() {
  uint64_t val = read();
  state::inputs::s_hex = to_hex_string(val, 8);
  state::inputs::s_int = std::to_string(*(int *) &val);
  state::inputs::s_long = std::to_string(val);
  state::inputs::s_float = std::to_string(*(float *) &val);
  state::inputs::s_double = std::to_string(*(double *) &val);
}

// same as sync_inputs(), but update register's value first
static void sync_inputs(uint64_t value) {
  write(value);
  sync_inputs();
}

// update register value from the given input field
static void update_reg_from_input(Field update) {
  uint64_t buffer;

  switch (update) {
    case Field::Hex: {
      try {
        buffer = std::stoull(state::inputs::s_hex, nullptr, 16);
      } catch (std::exception &e) {}
      break;
    }
    case Field::Int: {
      try {
        int value = std::stoi(state::inputs::s_int);
        buffer = *(uint32_t *) &value;
      } catch (std::exception &e) {}
      break;
    }
    case Field::Long: {
      try {
        long value = std::stoll(state::inputs::s_long);
        buffer = *(uint64_t *) &value;
      } catch (std::exception &e) {}
      break;
    }
    case Field::Float: {
      try {
        float value = std::stof(state::inputs::s_float);
        buffer = *(uint32_t *) &value;
      } catch (std::exception &e) {}
      break;
    }
    case Field::Double: {
      try {
        double value = std::stod(state::inputs::s_double);
        buffer = *(uint64_t *) &value;
      } catch (std::exception &e) {}
      break;
    }
  }

  sync_inputs(buffer);
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

// event handler for intercepted event in register_list
static bool register_list_on_event(ftxui::Event &e) {
  if (e.is_character()) {
    if (e == ftxui::Event::Character('r')) { // sync input fields
      sync_inputs();
      return true;
    }

    if (e == ftxui::Event::Character('c')) { // copy current register's contents
      state::copied_reg = read();
      return true;
    }

    if (e == ftxui::Event::Character('v')) { // paste register contents (if available)
      if (state::copied_reg.has_value()) {
        sync_inputs(state::copied_reg.value());
      }
      return true;
    }
  }

  if (e == ftxui::Event::Backspace || e == ftxui::Event::Delete) { // clear current register
    sync_inputs(0);
    return true;
  }

  return false;
}

// generate info for the $flag register, add to children (will be placed in a VBox)
static void flag_register_info(std::vector<ftxui::Element> &elements) {
  using namespace ftxui;

  std::vector<std::vector<Element>> entries; // table entries
  const uint64_t flag = visualiser::processor::cpu.reg(constants::registers::flag, true);

  // comparison bits
  if (auto it = std::find(state::flag::cmp_entries.begin(), state::flag::cmp_entries.end(), constants::cmp::to_string(
        static_cast<constants::cmp::flag>(flag & constants::cmp_bits))); it != state::flag::cmp_entries.end())
    state::flag::cmp_dropdown_select = it - state::flag::cmp_entries.begin();
  entries.emplace_back();
  entries.back().push_back(text("Bits 0-2"));
  entries.back().push_back(text("Comparison"));
  entries.back().push_back(state::flag::cmp_dropdown->Render());

  // zero flag
  state::flag::is_zero = flag & uint64_t(constants::flag::zero);
  entries.emplace_back();
  entries.back().push_back(text("Bit 3"));
  entries.back().push_back(text("Zero"));
  entries.back().push_back(hbox({
    state::flag::is_zero_toggle->Render(),
    text(" "),
    boolean(state::flag::is_zero) | (state::flag::is_zero_toggle->Focused() ? visualiser::style::highlight : nothing),
  }));

  // is running flag
  state::flag::is_running = flag & uint64_t(constants::flag::is_running);
  entries.emplace_back();
  entries.back().push_back(text("Bit 4"));
  entries.back().push_back(text("Is Running"));
  entries.back().push_back(hbox({
    state::flag::is_running_toggle->Render(),
    text(" "),
    boolean(state::flag::is_running) | (state::flag::is_running_toggle->Focused() ? visualiser::style::highlight : nothing),
  }));

  // error bits
  entries.emplace_back();
  entries.back().push_back(text("Bits 5-7"));
  entries.back().push_back(text("Error"));
  auto error_code = visualiser::processor::cpu.get_error();
  Element error_element = error_code ? (text("code " + std::to_string(error_code)) | visualiser::style::bad) : (text("none") | visualiser::style::ok);
  entries.back().push_back(error_element);

  // handling an interrupt?
  state::flag::in_interrupt = flag & int(constants::flag::in_interrupt);
  entries.emplace_back();
  entries.back().push_back(text("Bit 8"));
  entries.back().push_back(text("Handling Interrupt"));
  entries.back().push_back(boolean(state::flag::in_interrupt));

  // create the actual table
  Table table(entries);
  for (int i = 0; i < entries.size(); i++)
    table.SelectColumn(i).Border(LIGHT);
  elements.push_back(table.Render());
}

// change selection inside state::flag::cmp_dropdown
static void cmp_dropdown_on_change() {
  std::string& selected = state::flag::cmp_entries[state::flag::cmp_dropdown_select];
  if (auto flag = constants::cmp::map.find(selected); flag != constants::cmp::map.end()) {
    uint64_t new_value = visualiser::processor::cpu.reg(constants::registers::flag, true) | flag->second;
    visualiser::processor::cpu.reg_set(constants::registers::flag, new_value, true);
    sync_inputs();
  }
}

void visualiser::tabs::RegistersTab::init() {
  using namespace ftxui;

  for (int r = 0; r < constants::registers::count; r++) {
    state::reg_names.push_back("$" + constants::registers::to_string($reg(r)));
  }

  init_inputs();
  sync_inputs();

  auto register_list = Renderer(Menu(&state::reg_names, &state::current_reg),
                                [&] {
                                  std::vector<Element> children, left, right;

                                  for (int r = 0; r < constants::registers::count; r++) {
                                    auto name = text("$" + constants::registers::to_string($reg(r)));
                                    if (r == state::current_reg) name |= style::highlight;
                                    left.push_back(name);

                                    auto value = hbox({text(" = "),
                                                       text("0x" + to_hex_string(read(r), 8)) | style::reg});
                                    if (r == state::current_reg) value |= style::highlight;
                                    right.push_back(value);

                                    // divide special/general purpose registers
                                    if (r + 1 == constants::registers::r1) {
                                      children.push_back(hbox(vbox(left), vbox(right)));
                                      children.push_back(separator());
                                      left.clear();
                                      right.clear();
                                    }
                                  }

                                  if (!left.empty()) {
                                    children.push_back(hbox(vbox(left), vbox(right)));
                                  }

                                  return vbox(children) | border;
                                }) | CatchEvent([&](Event e) {
    return register_list_on_event(e);
  });

  // input controls for the $flag register
  state::flag::cmp_entries.reserve(constants::cmp::map.size());
  for (auto& [key, value] : constants::cmp::map) {
    state::flag::cmp_entries.push_back(key);
  }
  state::flag::cmp_dropdown = ftxui::Dropdown({
    .radiobox = {
        .entries = &state::flag::cmp_entries,
        .selected = &state::flag::cmp_dropdown_select,
        .on_change = cmp_dropdown_on_change
    }
  });

  state::flag::is_zero_toggle = create_checkbox("", [] {
    visualiser::processor::cpu.flag_toggle(constants::flag::zero, true);
    sync_inputs();
  }, state::flag::is_zero);

  state::flag::is_running_toggle = create_checkbox("", [] {
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
    sync_inputs();
  }, state::flag::is_running);

  Component register_view_layout = Container::Vertical({
    state::inputs::i_hex,
    state::inputs::i_int,
    state::inputs::i_long,
    state::inputs::i_float,
    state::inputs::i_double,
    Maybe(Container::Vertical({
      state::flag::cmp_dropdown,
      state::flag::is_zero_toggle,
      state::flag::is_running_toggle,
    }), [] { return state::current_reg == int(constants::registers::flag); })
  });

  auto register_view = Renderer(register_view_layout, [] {
        auto reg = $reg(state::current_reg);
        std::vector<Element> children{
                        center(text("Register $" + constants::registers::to_string(reg)) | bold),
                        separator(),
                        hbox({text("Value = "),
                              text("0x"),
                              state::inputs::i_hex->Render()}),
                        hbox({text("Integer = "),
                              state::inputs::i_int->Render()}),
                        hbox({text("Long = "),
                              state::inputs::i_long->Render()}),
                        hbox({text("Float = "),
                              state::inputs::i_float->Render()}),
                        hbox({text("Double = "),
                              state::inputs::i_double->Render()}),
                        separator(),
                        text(constants::registers::describe(reg).value()),
                    };
        if (reg == constants::registers::flag) {
          children.push_back(separator());
          flag_register_info(children);
        }
        return vbox(children) | border;
      });
  // TODO make description text wrap

  content_ = Container::Horizontal({register_list, register_view});

  help_ = Renderer([&] {
    return create_key_help_pane({
                                    {"Backspace/Delete", "zeroes the register"},
                                    {"c", "copies register contents"},
                                    {"v", "pastes contents into register"},
                                    {"r", "refresh page"},
                                    {"Up/Down", "move register selection"},
                                });
  });
}
