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
  std::vector<std::string> reg_names; // list of register names, used in menu
  int current_reg = 0; // index of the current register
  std::optional<uint64_t> copied_reg; // copied register contents

  namespace inputs {
    std::string s_hex, s_int, s_long, s_float, s_double;
    ftxui::Component i_hex, i_int, i_long, i_float, i_double;
  }// namespace inputs
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
  state::inputs::s_hex = to_hex_string(val);
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
  std::vector<std::string> location, fields;
  std::vector<Element> values;
  const uint64_t flag = visualiser::processor::cpu.reg(constants::registers::flag, true);
  uint64_t field;

  location.emplace_back("Bits 0-2");
  fields.emplace_back("Comparison");
  field = flag & constants::cmp_bits;
  values.push_back(text(constants::cmp::to_string(static_cast<constants::cmp::flag>(field))) | visualiser::style::value);

  location.emplace_back("Bit 3");
  fields.emplace_back("Zero");
  field = flag & uint64_t(constants::flag::zero);
  values.push_back(field ? (text("true") | visualiser::style::ok) : (text("false") | visualiser::style::bad));

  location.emplace_back("Bit 4");
  fields.emplace_back("Is Running");
  field = flag & uint64_t(constants::flag::is_running);
  values.push_back(field ? (text("true") | visualiser::style::ok) : (text("false") | visualiser::style::bad));

  location.emplace_back("Bits 5-7");
  fields.emplace_back("Error");
  field = visualiser::processor::cpu.get_error();
  values.push_back(field ? (text("code " + std::to_string(field)) | visualiser::style::bad) : (text("none") | visualiser::style::ok));

  location.emplace_back("Bit 8");
  fields.emplace_back("Handling Interrupt");
  field = flag & uint64_t(constants::flag::in_interrupt);
  values.push_back(field ? (text("true") | visualiser::style::ok) : (text("false") | visualiser::style::bad));

  // assembly into rows
  elements.push_back(hbox({
    vbox(map(location, std::function<Element(std::string&)>([](auto &s) { return text(s); }))),
    separator(),
    vbox(map(fields, std::function<Element(std::string&)>([](auto &s) { return text(s); }))),
    separator(),
    vbox(values)}) | border);
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
                                  std::vector<Element> left, right;

                                  for (int r = 0; r < constants::registers::count; r++) {
                                    auto name = text("$" + constants::registers::to_string($reg(r)));
                                    if (r == state::current_reg) name |= style::highlight_traced;
                                    left.push_back(name);

                                    auto value = hbox({text(" = "),
                                                       text("0x" + to_hex_string(read(r))) | style::reg});
                                    if (r == state::current_reg) value |= style::highlight_traced;
                                    right.push_back(value);
                                  }

                                  return hbox({vbox(left), vbox(right)}) | border;
                                }) | CatchEvent([&](Event e) {
    return register_list_on_event(e);
  });

  auto register_view = Renderer(
      Container::Vertical({state::inputs::i_hex, state::inputs::i_int, state::inputs::i_long, state::inputs::i_float,
                           state::inputs::i_double}),
      [&] {
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
