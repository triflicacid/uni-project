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

// replace '\r\n' with just '\n'
static std::string normalise_newlines(const std::string &input) {
  return std::regex_replace(input, std::regex("\r\n"), "\n");
}

static std::string strip_ansi(const std::string &input) {
  return std::regex_replace(input, std::regex("\e\\[[0-9;]*[a-zA-Z]"), "");
}

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

// update the state's debug message list
static void update_debug_lines() {
  if (!processor::debug::any()) return;

  std::string debug = visualiser::processor::debug_stream.str();
  debug = strip_ansi(normalise_newlines(debug));

  state::debug_lines.clear();
  std::istringstream stream(debug);
  std::string line;
  while (std::getline(stream, line)) state::debug_lines.push_back(ftxui::text(line));
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
                                vbox(std::move(elements)) | frame | border,
                            });

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
                             vbox(std::move(elements)) | frame | border,
                         });

    // pane - information
    elements.clear();
    if (::processor::debug::any() && !state::debug_lines.empty())
      elements.push_back(vbox(state::debug_lines) | border);
    processor::debug_stream.str("");
    processor::debug_stream.clear();

    elements.push_back(hbox({
                                text("Status: "),
                                processor::cpu.is_running()
                                ? text("Running") | color(Color::GreenLight)
                                : text("Halted") | color(Color::RedLight),
                            }));
    elements.push_back(hbox({
                                text("Cycle: "),
                                text(std::to_string(state::current_cycle)) | color(style::value),
                            }));
    elements.push_back(hbox({
                                text("$pc: "),
                                text("0x" + to_hex_string(state::current_pc)) | color(style::reg),
                            }));

    auto info_pane = vbox(elements);

    // return pane combination
    return vbox({hbox({source_pane,
                       separator(),
                       asm_pane}),
                 separator(),
                 info_pane});
  });

  content_ |= CatchEvent([&](Event event) {
    return on_event(event);
  });
}
