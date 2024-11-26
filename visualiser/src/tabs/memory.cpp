#include "memory.hpp"
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
  uint64_t base_address = 0;
  uint64_t current_addr = 0;

  std::array<std::array<std::pair<ftxui::Component, std::string>, 16>, 6> inputs;
}// namespace state

// read the given address
static uint64_t read(uint64_t address, uint8_t bytes) {
  return visualiser::processor::cpu.mem_load(address, bytes);
}

// write to the given address
static void write(uint64_t address, uint8_t bytes, uint64_t value) {
  visualiser::processor::cpu.mem_store(address, bytes, value);
}

// initialise inputs
static void init_inputs() {
  uint64_t address = 0, value;

  for (auto &row : state::inputs) {
    for (auto &[input, string] : row) {
      value = read(state::base_address + address, 1);
      string = to_hex_string(value, 1);
      input = ftxui::Input({
        .content = &string,
        .multiline = false,
        .on_enter = [&string, address] {
          try {
            uint64_t value = std::stoul(string, nullptr, 16);
            write(state::base_address + address, 1, value);
          } catch (std::exception &e) { }
        }
      });
      address++;
    }
  }
}

// update contents of the inputs
static void sync_inputs() {
  uint64_t address = 0, value;

  for (auto &row : state::inputs) {
    for (auto &[input, string] : row) {
      value = read(state::base_address + address, 1);
      string = to_hex_string(value, 1);
      address++;
    }
  }
}

void visualiser::tabs::MemoryTab::init() {
  using namespace ftxui;

  init_inputs();

  // create grid of inputs
  std::vector<Component> children;
  for (auto &row : state::inputs) {
    std::vector<Component> sub_children;
    for (auto &[input, _] : row)
      sub_children.push_back(input);
    children.push_back(Container::Horizontal(sub_children));
  }
  Component memory_container = Container::Vertical(children);

  content_ = Container::Vertical({
    Renderer([&] {
      return hbox({
        text("Base Address: "),
        text("0x" + to_hex_string(state::base_address, 4)) | visualiser::style::value
      }) | center | border;
    }),
    Renderer(memory_container, [&] {
      uint32_t address = 0;
      std::vector<std::vector<Element>> grid_elements;

      // column headers
      grid_elements.emplace_back();
      auto *elements = &grid_elements.back();
      elements->push_back(text(""));
      elements->push_back(text(""));
      for (const auto &_ : state::inputs[0])
        elements->push_back(text("+" + to_hex_string(address++, 1) + " ") | style::value | center);

      // add separators
      grid_elements.emplace_back();
      elements = &grid_elements.back();
      elements->push_back(text(""));
      elements->push_back(text(""));
      for (const auto &_ : state::inputs[0])
        elements->push_back(separator());

      // memory grid
      address = state::base_address;
      for (auto &row : state::inputs) {
        grid_elements.emplace_back();
        elements = &grid_elements.back();
        elements->push_back(text("0x" + to_hex_string(address, 4)) | style::value); // row start address
        elements->push_back(separator());
        for (auto &[input, _] : row)
          elements->push_back(input->Render());
        address += row.size();
      }

      return gridbox(grid_elements);
    }),
  });

  help_ = Renderer([&] {
    return create_key_help_pane({
                                    {"r", "refresh page"},
                                });
  });
}
