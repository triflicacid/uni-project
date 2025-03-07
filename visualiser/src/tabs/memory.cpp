#include "memory.hpp"
#include "processor.hpp"
#include "style.hpp"
#include "util.hpp"
#include "components/custom_dropdown.hpp"
#include <ftxui/component/event.hpp>

enum class InputType : int {
  HexInt,
  DecInt,
  HexLong,
  DecLong,
  Float,
  Double
};

namespace state {
  int constexpr rows = 16;
  int constexpr cols = 16;
  int constexpr page_size = rows * cols;

  static bool highlight_special_registers = true; // highlight $PC, $SP, $FP
  static ftxui::Component memory_grid, memory_address_editor;
  static uint64_t base_address = 0;
  static std::pair<int, int> pos{0, 0}; // current position (x, y)

  static ftxui::Component mem_input;
  static std::string mem_input_content;
  static std::vector<std::string> input_type_values{"int 0x", "int ", "long 0x", "long ", "float ", "double "};
  static ftxui::Component input_type_dropdown;
  static InputType input_type_index;

  // return the current selected address
  static uint64_t current_address() {
    return base_address + pos.second * cols + pos.first;
  }

  ftxui::Decorator& pc_colour = visualiser::style::highlight_execution;
  ftxui::Decorator sp_colour = ftxui::bgcolor(ftxui::Color::Violet) | ftxui::color(ftxui::Color::Black);
  ftxui::Decorator fp_colour = ftxui::bgcolor(ftxui::Color::BlueViolet) | ftxui::color(ftxui::Color::White);
}// namespace state

// read the given address
static uint64_t read(uint64_t address, uint8_t bytes) {
  return visualiser::processor::cpu.mem_load(address, bytes);
}

// write to the given address
static void write(uint64_t address, uint8_t bytes, uint64_t value) {
  visualiser::processor::cpu.mem_store(address, bytes, value);
}

// shift and write to lower nibble of address
static void shift_and_write_nibble(uint64_t address, uint8_t byte) {
  uint64_t value = read(address, 1);
  value = (value << 4) | (byte & 0xf);
  write(address, 1, value);
}

// validate and update state::pos, return if anything changed
static bool validate_pos() {
  using namespace state;
  bool was_change = false;
  // check column (x) index
  if (pos.first < 0) { // wrap around to last row
    pos.first = cols - 1;
    pos.second--;
    was_change = true;
  } else if (pos.first >= cols) { // wrap around to next row
    pos.first = 0;
    pos.second++;
    was_change = true;
  }

  // check row (y) index
  if (pos.second < 0) { // shift base address down if possible
    pos.second = 0;
    if (state::base_address > 0) state::base_address -= cols;
    was_change = true;
  } else if (pos.second >= rows) { // shift base address down if possible
    pos.second = rows - 1;
    state::base_address += cols;
    if (state::base_address + state::page_size >= ::processor::dram::size) state::base_address = ::processor::dram::size - state::page_size;
    was_change = true;
  }

  return was_change;
}

// translate state::pos by delta and validate
static bool move_pos(int dx, int dy) {
  state::pos.first += dx;
  state::pos.second += dy;
  return validate_pos();
}

// return number of bytes we are reading given the InputType
static int get_number_of_bytes() {
  switch (state::input_type_index) {
    case InputType::HexInt:
    case InputType::DecInt:
    case InputType::Float:
      return 4;
    case InputType::HexLong:
    case InputType::DecLong:
    case InputType::Double:
      return 8;
    default:
      return 0;
  }
}

// update state::mem_input
static void sync_mem_input() {
  uint64_t value = read(state::current_address(), get_number_of_bytes());
  switch (state::input_type_index) {
    case InputType::HexInt:
      state::mem_input_content = to_hex_string(*(uint32_t*)&value, 4);
      break;
    case InputType::DecInt:
      state::mem_input_content = std::to_string(*(int32_t*)&value);
      break;
    case InputType::HexLong:
      state::mem_input_content = to_hex_string(value, 8);
      break;
    case InputType::DecLong:
      state::mem_input_content = std::to_string(*(int64_t*)&value);
      break;
    case InputType::Float:
      state::mem_input_content = std::to_string(*(float*)&value);
      break;
    case InputType::Double:
      state::mem_input_content = std::to_string(*(double*)&value);
      break;
  }
}

// called when state::mem_input is submitted
static void update_mem_input() {
  uint64_t value;
  uint8_t bytes;
  switch (state::input_type_index) {
    case InputType::HexInt:
      try {
        value = std::stoul(state::mem_input_content, nullptr, 16);
        bytes = 4;
      } catch (std::exception &e) { }
      break;
    case InputType::DecInt:
      try {
        int raw = std::stoi(state::mem_input_content);
        value = *(uint32_t*)&raw;
        bytes = 4;
      } catch (std::exception &e) { }
      break;
    case InputType::HexLong:
      try {
        value = std::stoul(state::mem_input_content, nullptr, 16);
        bytes = 8;
      } catch (std::exception &e) { }
      break;
    case InputType::DecLong:
      try {
        long raw = std::stol(state::mem_input_content);
        value = *(uint64_t*)&raw;
        bytes = 8;
      } catch (std::exception &e) { }
      break;
    case InputType::Float:
      try {
        float raw = std::stof(state::mem_input_content);
        value = *(uint32_t*)&raw;
        bytes = 4;
      } catch (std::exception &e) { }
      break;
    case InputType::Double:
      try {
        double raw = std::stod(state::mem_input_content);
        value = *(uint64_t*)&raw;
        bytes = 8;
      } catch (std::exception &e) { }
      break;
    default:
      return;
  }

  write(state::current_address(), bytes, value);
}

// catch event in memory grid
static bool memory_grid_on_event(const ftxui::Event& e) {
  using namespace ftxui;
//  if (!e.is_mouse()) {
//    std::cerr << e.DebugString() << " | {";
//    for (char c : e.input()) std::cerr << int(c) << ", ";
//    std::cerr << "}" << std::endl;
//  }

  if (e == Event::Return) {
    state::mem_input->TakeFocus();
    sync_mem_input();
  } else if (e == Event::ArrowLeft) {
    move_pos(-1, 0);
    sync_mem_input();
  } else if (e == Event::ArrowRight) {
    move_pos(1, 0);
    sync_mem_input();
  } else if (e == Event::ArrowDown) {
    move_pos(0, 1);
    sync_mem_input();
  } else if (e == Event::ArrowUp) {
    move_pos(0, -1);
    sync_mem_input();
  } else if (e == Event::Home) {
    state::pos.first = 0;
    sync_mem_input();
  } else if (e == Event::End) {
    state::pos.first = state::cols - 1;
    sync_mem_input();
  } else if (e == Event::PageDown) {
    move_pos(0, state::rows);
    sync_mem_input();
  } else if (e == Event::Special({27, 91, 49, 59, 53, 70})) { // Ctrl+End
    state::base_address = ::processor::dram::size - state::rows * state::cols;
    sync_mem_input();
  } else if (e == Event::PageUp) {
    move_pos(0, -state::rows);
    sync_mem_input();
  } else if (e == Event::Special({27, 91, 49, 59, 53, 72})) { // Ctrl+Home
    state::base_address = 0;
    sync_mem_input();
  } else if (e == Event::Backspace || e == Event::Delete) { // clear the given address
    write(state::current_address(), 1, 0x0);
    sync_mem_input();
  } else if (e.is_character()) {
    auto ch = e.character()[0];
    if (std::isdigit(ch)) {
      shift_and_write_nibble(state::current_address(), ch - '0');
    } else if (ch >= 'a' && ch <= 'f') {
      shift_and_write_nibble(state::current_address(), ch - 'a' + 10);
    } else if (ch >= 'A' && ch <= 'F') {
      shift_and_write_nibble(state::current_address(), ch - 'A' + 10);
    } else {
      return false;
    }
    sync_mem_input();
    return true;
  } else {
    return false;
  }
  return true;
}

void visualiser::tabs::MemoryTab::init() {
  using namespace ftxui;

  // initialise input fields
  state::input_type_dropdown = CustomDropdown(&state::input_type_values, (int*)&state::input_type_index, {
    .radiobox = {
        .on_change = sync_mem_input
    }
  });
  state::mem_input = Input({
    .content = &state::mem_input_content,
    .multiline = false,
    .on_enter = update_mem_input
  });

  state::memory_grid = Renderer([&](bool focused) {
    uint32_t address = 0;
    std::vector<std::vector<Element>> grid_elements;

    // column headers
    grid_elements.emplace_back();
    auto *elements = &grid_elements.back();
    elements->push_back(text(""));
    elements->push_back(text(""));
    for (int i = 0; i < state::cols; i++)
      elements->push_back(text("+" + to_hex_string(address++, 1).substr(1) + " ") | style::value | center);

    // add separators
    grid_elements.emplace_back();
    elements = &grid_elements.back();
    elements->push_back(text(""));
    elements->push_back(text(""));
    for (int i = 0; i < state::cols; i++)
      elements->push_back(separator());

    // memory grid
    address = state::base_address;
    for (int y = 0; y < state::rows; y++) {
      grid_elements.emplace_back();
      elements = &grid_elements.back();
      elements->push_back(text(to_hex_string(address, 4)) | style::value | align_right); // row start address
      elements->push_back(separator());
      for (int x = 0; x < state::cols; x++, address++) {
        elements->push_back(text(to_hex_string(read(address, 1), 1)));
      }
    }

    if (focused) {
      // highlight the selected cell
      grid_elements[2 + state::pos.second][2 + state::pos.first] |= style::highlight;
    } else if (state::memory_address_editor->Focused()) {
      // highlight the bytes we are reading from
      const int bytes = get_number_of_bytes();
      address = state::current_address();
      for (int i = 0, x = state::pos.first, y = state::pos.second; i < bytes; i++, address++) {
        grid_elements[2 + y][2 + x] |= style::highlight;

        if (++x >= state::cols) {
          x = 0;
          y++;
          if (y >= state::rows) break; // have we now exceeded the memory grid?
        }
      }
    }

    // highlight cells of the special registers?
    uint64_t $pc = processor::cpu.read_pc();
    uint64_t $sp = processor::cpu.reg(constants::registers::sp, true);
    uint64_t $fp = processor::cpu.reg(constants::registers::fp, true);
    if (state::highlight_special_registers) {
      if (int offset = int($pc) - int(state::base_address); $pc >= state::base_address && offset < state::page_size) {
        grid_elements[2 + offset / state::cols][2 + offset % state::cols] |= state::pc_colour;
      }
      if (int offset = int($sp) - int(state::base_address); $sp >= state::base_address && offset < state::page_size) {
        grid_elements[2 + offset / state::cols][2 + offset % state::cols] |= state::sp_colour;
      }
      if (int offset = int($fp) - int(state::base_address); $fp != $sp && $fp >= state::base_address && offset < state::page_size) {
        grid_elements[2 + offset / state::cols][2 + offset % state::cols] |= state::fp_colour;
      }
    }

    return vbox({
      hbox({
               text("Memory View: "),
               text("0x" + to_hex_string(state::base_address, 4)) | visualiser::style::value,
               text(" – "),
               text("0x" + to_hex_string(state::base_address + state::page_size, 4)) | visualiser::style::value,
           }) | center,
      gridbox(grid_elements) | border | center,
      hbox({
               hbox(text("$pc = "), text("0x" + to_hex_string(*(uint64_t*)&$pc, 8)) | state::pc_colour),
               separator(),
               hbox(text("$sp = "), text("0x" + to_hex_string(*(uint64_t*)&$sp, 8)) | state::sp_colour),
               separator(),
               hbox(text("$fp = "), text("0x" + to_hex_string(*(uint64_t*)&$fp, 8)) | state::fp_colour)
           })
    });
  });

  state::memory_grid |= CatchEvent(memory_grid_on_event);

  state::memory_address_editor = Renderer(Container::Horizontal({
    state::input_type_dropdown,
    state::mem_input,
  }), [&]{
    return hbox({
      text("Address "),
      text("0x" + to_hex_string(state::current_address(), 4)) | style::value,
      text(" = "),
      state::input_type_dropdown->Render(),
      state::mem_input->Render(),
    });
  });

  content_ = Container::Vertical({
    state::memory_grid,
    Renderer([]{ return separator(); }),
    state::memory_address_editor,
  });

  help_ = Renderer([&] {
    return create_key_help_pane({
                                    {"Arrows", "move selection"},
                                    {"PageUp/Down", "move one page up/down"},
                                    {"Ctrl+Home/End", "move to start/end of memory"},
                                    {"Home/End", "move to start/end of line"},
                                    {"Backspace/Delete", "zero memory location"},
                                    {"Enter", "jump to input box"},
                                });
  });
}
