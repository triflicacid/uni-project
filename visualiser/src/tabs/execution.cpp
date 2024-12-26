#include "execution.hpp"
#include "assembly.hpp"
#include "processor.hpp"
#include "screen.hpp"
#include "style.hpp"
#include "util.hpp"
#include <climits>
#include <regex>


enum class Pane {
  Assembly,
  Source,
};

namespace state {
  uint64_t current_pc = 0;                          // current value of $pc
  int current_cycle = 0;                            // processor's current cycle
  visualiser::assembly::PCEntry *pc_entry = nullptr;// current PC entry
  std::vector<ftxui::Element> debug_lines; // contains lines in the debug field - preserves values

  std::vector<std::string> source_lines, asm_lines; // lines in each pane
  bool show_selected_line = false; // show light-blue selected line(s)?
  std::pair<Pane, int> selected_line = {Pane::Source,
                                        1};// selected line in the corresponding source: <pane, line> (line is 1-indexed)

  std::deque<int> source_selected_lines, asm_selected_lines; // store selected lines in each pane
  ftxui::Decorator *source_selected_style, *asm_selected_style; // which style to use when colouring selected lines? (trace-back)
}// namespace state

// get the current pane's line limit
static int get_pane_limit(Pane pane) {
  switch (pane) {
    case Pane::Source:
      return state::source_lines.size();
    case Pane::Assembly:
      return state::asm_lines.size();
    default:
      return 0;
  }
}

// update selected line information - trace lines from current pane to others
static void update_selected_line() {
  using namespace state;
  if (!show_selected_line) return;

  clamp(state::selected_line.second, 1, get_pane_limit(state::selected_line.first) + 1);
  source_selected_lines.clear();
  asm_selected_lines.clear();
  auto &[pane, line] = state::selected_line;

  if (pane == Pane::Source) {
    source_selected_lines.push_back(line);
    auto location = visualiser::assembly::locate_line(line);
    if (location) asm_selected_lines.push_back(location->origin.line());

    source_selected_style = &visualiser::style::highlight_selected;
    asm_selected_style = &visualiser::style::highlight_traced;
  } else if (pane == Pane::Assembly) {
    asm_selected_lines.push_back(line);
    asm_selected_style = &visualiser::style::highlight_selected;
    source_selected_style = &visualiser::style::highlight_traced;

    auto locations = visualiser::assembly::locate_asm_line(state::pc_entry->origin.path(), line);
    for (auto &location: locations)
      source_selected_lines.push_back(location->line_no);
  }
}

// given debug message, return Element which represents it
static ftxui::Element format_debug_message(const processor::debug::Message &msg) {
  using namespace ftxui;
  using namespace processor::debug;
  std::vector<Element> children; // elements to be placed in an HBox
  std::stringstream os;

  switch (msg.type) {
    case Message::Cycle: {
      auto *message = (CycleMessage*)&msg;
      if (message->n < 0) os << "step";
      else os << "cycle #" << message->n + 1;
      children.push_back(text(os.str()) | color(Color::Violet));
      empty_stream(os);
      os << ": $pc=0x" << std::hex << message->pc << ", inst=0x" << message->inst << std::dec;
      children.push_back(text(os.str()));
      break;
    }
    case Message::Instruction: {
      auto *message = (InstructionMessage*)&msg;
      os << message->mnemonic << ": " << message->message.str();
      children.push_back(text(os.str()));
      break;
    }
    case Message::Argument: {
      auto *message = (ArgumentMessage*)&msg;
      os << "arg #" << message->n;
      children.push_back(text(os.str()) | color(Color::Cyan));
      empty_stream(os);
      children.push_back(text(": "));
      children.push_back(text(constants::inst::arg_to_string(message->arg_type) + " ") | color(Color::LightSteelBlue));
      children.push_back(text(message->message.str()));
      children.push_back(text(" -> ") | color(Color::LightSteelBlue));
      os << "0x" << std::hex << message->value << std::dec;
      children.push_back(text(os.str()));
      break;
    }
    case Message::Register: {
      auto *message = (RegisterMessage*)&msg;
      children.push_back(text("reg") | color(Color::LightYellow3));
      os << ": ";
      if (message->is_write) os << "set $" << constants::registers::to_string(message->reg) << " to ";
      else os << "access $" << constants::registers::to_string(message->reg) << " -> ";
      os << "0x" << std::hex << message->value << std::dec;
      children.push_back(text(os.str()));
      break;
    }
    case Message::Memory: {
      auto *message = (MemoryMessage*)&msg;
      children.push_back(text("mem") | color(Color::Yellow3));
      if (message->is_write) os << ": store data 0x" << std::hex << message->value << " of " << std::dec << (int) message->bytes << " bytes at address 0x" << std::hex << message->address << std::dec;
      else os << ": access " << (int) message->bytes << " bytes from address 0x" << std::hex << message->address << " -> 0x" << message->value << std::dec;
      children.push_back(text(os.str()));
      break;
    }
    case Message::ZeroFlag: {
      auto *message = (ZeroFlagMessage*)&msg;
      children.push_back(text("zero flag") | color(Color::Cyan));
      os << ": register $" << constants::registers::to_string(message->reg) << " -> ";
      children.push_back(text(os.str()));
      if (message->state) children.push_back(text("set") | visualiser::style::ok);
      else children.push_back(text("reset") | visualiser::style::bad);
      break;
    }
    case Message::Conditional: {
      auto *message = (ConditionalMessage*)&msg;
      children.push_back(text("conditional") | color(Color::Cyan));
      os << ": " << constants::cmp::to_string(message->test_bits) << " -> ";
      children.push_back(text(os.str()));
      empty_stream(os);

      if (message->passed) children.push_back(text("pass") | visualiser::style::ok);
      else {
        children.push_back(text("fail") | visualiser::style::bad);
        os << " ($flag: 0x" << std::hex << (int) message->flag_bits.value() << std::dec << ")";
        children.push_back(text(os.str()));
      }
      break;
    }
    case Message::Interrupt: {
      auto *message = (InterruptMessage*)&msg;
      children.push_back(text("interrupt!") | color(Color::CadetBlue));
      os << "$isr=0x" << std::hex << message->isr << ", $imr=0x" << message->imr << ", %iip=0x" << message->ipc << std::dec;
      children.push_back(text(os.str()));
      break;
    }
    case Message::Error: {
      auto *message = (ErrorMessage*)&msg;
      children.push_back(text(message->message) | visualiser::style::error);
      break;
    }
    default:
      children.push_back(text("Unknown message") | visualiser::style::bad);
  }

  return children.size() == 1 ? children.front() : hbox(children);
}

// update the state's debug message list
static void update_debug_lines() {
  // format CPU's debug messages
  state::debug_lines.clear();
  for (const auto &msg : visualiser::processor::cpu.get_debug_messages())
    state::debug_lines.push_back(format_debug_message(*msg));
  visualiser::processor::cpu.clear_debug_messages();

  // add error message to debug, if there is an error
  if (visualiser::processor::cpu.get_error()) {
    std::ostringstream es;
    visualiser::processor::cpu.print_error(es, false);
    state::debug_lines.push_back(ftxui::text(es.str()) | visualiser::style::error);
  }
}

namespace events {
  static bool on_reset() {
    visualiser::processor::cpu.reset_flag();
    return true;
  }

  static bool on_enter() {
    using namespace visualiser::processor;
    if (cpu.is_running()) {
      cpu.step(state::current_cycle);
      state::current_pc = cpu.read_pc();
      update_debug_lines();
    } else {
      state::debug_lines.clear();
    }

    return true;
  }

  static bool on_vert_arrow(bool down) {
    if (!state::show_selected_line) return false;
    state::selected_line.second += down ? 1 : -1;
    update_selected_line();
    return true;
  }

  static bool on_horiz_arrow(bool right) {
    if (!state::show_selected_line) return false;
    switch (state::selected_line.first) {
      case Pane::Source:
        if (!right) {
          state::selected_line.first = Pane::Assembly;
          state::selected_line.second = state::asm_selected_lines.empty() ? 0 : state::asm_selected_lines.front();
          update_selected_line();
        }
        break;
      case Pane::Assembly:
        if (right) {
          state::selected_line.first = Pane::Source;
          state::selected_line.second = state::source_selected_lines.empty() ? 0 : state::source_selected_lines.front();
          update_selected_line();
        }
        break;
      default:
        return false;
    }
    return true;
  }

  static bool on_J() {
    if (state::show_selected_line && !state::source_selected_lines.empty()) {
      if (auto entry = visualiser::assembly::locate_line(state::source_selected_lines.front())) {
        state::current_pc = entry->pc;
        visualiser::processor::cpu.write_pc(entry->pc);
        return true;
      }
    }
    return false;
  }

  static bool on_H() { // toggle IS_RUNNING bit in $flag
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
    return true;
  }

  static bool on_S() {
    state::show_selected_line = !state::show_selected_line;
    update_selected_line();
    return true;
  }

  static bool on_home() {
    state::selected_line.second = 0;
    update_selected_line();
    return true;
  }

  static bool on_end() {
    state::selected_line.second = INT_MAX;
    update_selected_line();
    return true;
  }
}// namespace events

static bool on_event(ftxui::Event &event) {
  using namespace ftxui;
  using namespace events;

  if (event == Event::Return) return on_enter();
  if (event.is_character() && event == Event::Character('j')) return on_J();
  if (event.is_character() && event == Event::Character('h')) return on_H();
  if (event.is_character() && event == Event::Character('s')) return on_S();
  if (event.is_character() && event == Event::Character('r')) return on_reset();
  if (event == Event::Home) return on_home();
  if (event == Event::End) return on_end();
  if (event == Event::ArrowDown || event == Event::ArrowUp) return on_vert_arrow(event == Event::ArrowDown);
  if (event == Event::ArrowLeft || event == Event::ArrowRight) return on_horiz_arrow(event == Event::ArrowRight);
  return false;
}

void visualiser::tabs::CodeExecutionTab::init() {
  using namespace ftxui;

  // NOTE takes arg - focusable
  content_ = Renderer([&](bool _) {
    // get source of $pc, update our state if we have a value
    if (auto pc_entry = assembly::locate_pc(state::current_pc))
      state::pc_entry = pc_entry;

    // pane - view source file
    std::vector<Element> elements;
    auto &lines = state::asm_lines = assembly::get_file_lines(state::pc_entry->origin.path());
    elements.reserve(lines.size());
    for (const auto &line: lines)
      elements.push_back(text(line));

    elements[state::pc_entry->origin.line() - 1] |= style::highlight_execution;

    if (state::show_selected_line)
      for (const auto &line: state::asm_selected_lines)
        elements[line - 1] |= *state::asm_selected_style;

    auto source_pane = vbox({
                                text(state::pc_entry->origin.path()) | bold,
                                separator(),
                                vbox(std::move(elements)),
                            }) | border;

    // pane - view assembly source
    elements.clear();
    lines = state::source_lines = assembly::get_file_lines(assembly::source->path);
    for (const auto &line: lines)
      elements.push_back(text(line));

    elements[state::pc_entry->line_no - 1] |= style::highlight_execution;

    if (state::show_selected_line)
      for (auto &line: state::source_selected_lines)
        elements[line - 1] |= *state::source_selected_style;

    auto asm_pane = vbox({
                             text("Compiled Assembly") | bold,
                             separator(),
                             vbox(std::move(elements)),
                         }) | border;

    // pane - information
    elements.clear();
    if (!state::debug_lines.empty())
      elements.push_back(vbox(state::debug_lines) | border);

    std::vector<Element> children{text("Status: ")};
    if (processor::cpu.is_running()) {
      children.push_back(text("Running") | style::ok);
    } else if (processor::cpu.get_error()) {
      children.push_back(text("Halted (error)") | style::error);
    } else {
      children.push_back(text("Halted") | style::bad);
    }
    elements.push_back(hbox(children));

    elements.push_back(hbox({
                                text("Cycle: "),
                                text(std::to_string(state::current_cycle)) | style::value,
                            }));
    elements.push_back(hbox({
                                text("$pc: "),
                                text("0x" + to_hex_string(state::current_pc, 8)) | style::reg,
                            }));

    auto info_pane = vbox(elements);

    // return pane combination
    return vbox({
                    hbox({source_pane, asm_pane}),
                    info_pane});
  });

  content_ |= CatchEvent([&](Event event) {
    return on_event(event);
  });

  help_ = Renderer([&] {
    return create_key_help_pane({
      {"End", "select last line"},
      {"Enter", "execute one step"},
      {"h", "start/stop processor"},
      {"Home", "select first line"},
      {"j", "jump to selected line"},
      {"r", "reset the processor's state"},
      {"s", "toggle line select"},
      {"Up/Down", "move selected line"},
      {"Left/Right", "move selected line's pane"}
    });
  });
}
