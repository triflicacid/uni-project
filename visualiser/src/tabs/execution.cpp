#include "execution.hpp"
#include "assembly.hpp"
#include "processor.hpp"
#include "style.hpp"
#include "util.hpp"
#include "components/scroller.hpp"
#include "components/checkbox.hpp"
#include <ftxui/component/event.hpp>

enum class Pane {
  Assembly,
  Source,
};

struct PaneStateData {
  Pane type;
  ftxui::Component component;
  std::vector<std::string> lines; // lines in each pane
  int *pos_ptr; // pointer top ScrollerBase::selected_
  std::deque<int> selected_lines; // store selected lines in each pane

  PaneStateData(Pane type) : type(type) {}
};

struct Breakpoint {
  Pane type;
  Location origin;
  uint64_t pc;
};

namespace state {
  uint64_t current_pc = 0;                          // current value of $pc
  int current_cycle = 0;                            // processor's current cycle
  visualiser::assembly::PCEntry *pc_entry = nullptr;// current PC entry
  std::vector<ftxui::Element> debug_lines; // contains lines in the debug field - preserves values

  bool show_selected_line = false; // show light-blue selected line(s)?
  ftxui::Component show_selected_line_toggle; // toggle for show_selected_line Boolean
  bool is_running = false;
  ftxui::Component is_running_toggle; // toggle for IS_RUNNING
  bool do_update_selected_line = false;

  std::set<uint64_t> breakpoint_pcs; // set of $pc breakpoints
  std::deque<Breakpoint> breakpoints;

  PaneStateData source_pane(Pane::Source);
  PaneStateData asm_pane(Pane::Assembly);
  std::array<PaneStateData*, 2> panes{&source_pane, &asm_pane};
  PaneStateData* selected_pane = nullptr;
  int selected_line = 0; // current line which is selected
}// namespace state

// update panes' positions based on state::current_pc
static void update_align_pane_pc() {
  // locate $pc in assembly file
  if (auto pc_entry = visualiser::assembly::locate_pc(state::current_pc))
    state::pc_entry = pc_entry;
  else return;

  // align source pane
  *state::source_pane.pos_ptr = state::pc_entry->line_no;

  // align assembly pane
  *state::asm_pane.pos_ptr = state::pc_entry->asm_origin.line();
}

// update state::selected_pane
static void update_selected_pane() {
  state::selected_pane =
    state::source_pane.component->Focused() ? &state::source_pane
  : state::asm_pane.component->Focused() ? &state::asm_pane
  : nullptr;
}

// update selected line information - trace lines from current pane to others
static void update_selected_line() {
  using namespace state;
  update_selected_pane();
  if (!show_selected_line || !selected_pane) return;

  // ensure selected line is in bounds
  clamp(selected_line, 1, (int) selected_pane->lines.size() + 1);

  // clear selection
  for (PaneStateData* pane : state::panes)
    pane->selected_lines.clear();

  // select the current line
  selected_pane->selected_lines.push_back(selected_line);

  if (selected_pane->type == Pane::Source) {
    auto location = visualiser::assembly::locate_line(selected_line);
    if (location)
      asm_pane.selected_lines.push_back(location->asm_origin.line());
  } else if (selected_pane->type == Pane::Assembly) {
    auto locations = visualiser::assembly::locate_asm_line(pc_entry->asm_origin.path(), selected_line);
    for (auto &location: locations)
      source_pane.selected_lines.push_back(location->line_no);
  }
}

// ensure the selected lines in each pane are in view
static void update_pane_positions_from_selection() {
  for (PaneStateData* pane : state::panes) {
    auto min = std::min_element(pane->selected_lines.begin(), pane->selected_lines.end());
    if (min != pane->selected_lines.end()) {
      *pane->pos_ptr = *min;
    }
  }
}

// get the line highlight colour for the given pane
static ftxui::Decorator* get_line_highlight_style(Pane pane) {
  return state::selected_pane && pane == state::selected_pane->type ? &visualiser::style::highlight_selected : &visualiser::style::highlight_traced;
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
      break;
  }

  return children.size() == 1 ? children.front() : hbox(children);
}

// update $pc, but only in state:: -- use CPU's $pc
static void update_pc() {
  uint64_t pc = visualiser::processor::cpu.read_pc();
  state::current_pc = pc;

  // attempt to trace new PC
  if (auto pc_entry = visualiser::assembly::locate_pc(pc))
    state::pc_entry = pc_entry;
}

// update $pc -- sets both CPU's actual pc, state::current_pc, and state::pc_entry
static void update_pc(uint64_t val) {
  state::current_pc = val;
  visualiser::processor::cpu.write_pc(val);

  // attempt to trace new PC
  if (auto pc_entry = visualiser::assembly::locate_pc(val))
    state::pc_entry = pc_entry;
}

// update the state's debug message list
static void update_debug_lines() {
  // format CPU's debug messages
  state::debug_lines.clear();
  for (const auto &msg : visualiser::processor::cpu.get_debug_messages())
    state::debug_lines.push_back(format_debug_message(*msg));
//  state::debug_lines.push_back(ftxui::text("Debug messages: " + std::to_string(visualiser::processor::cpu.get_debug_messages().size())));
  visualiser::processor::cpu.clear_debug_messages();

  // add error message to debug, if there is an error
  if (visualiser::processor::cpu.get_error()) {
    std::ostringstream es;
    visualiser::processor::cpu.print_error(es, false);
    state::debug_lines.push_back(ftxui::text(es.str()) | visualiser::style::error);
  }
}

// get the current location $pc is in the given pane
static std::optional<Location> get_current_location(Pane pane) {
  if (!state::pc_entry) return {};
  switch (pane) {
    case Pane::Source:
      return Location("[source]", state::pc_entry->line_no);
    case Pane::Assembly:
      return state::pc_entry->asm_origin;
    default:
      return {};
  }
}

// get the current location selection is on in the given pane, and $pc
static std::optional<std::pair<Location, uint64_t>> get_selected_location() {
  if (!state::selected_pane) return {};
  auto pc_entry = visualiser::assembly::locate_line(state::source_pane.selected_lines.front());

  switch (state::selected_pane->type) {
    case Pane::Source:
      return std::pair{Location("[source]", pc_entry->line_no), pc_entry->pc};
    case Pane::Assembly:
      return std::pair{pc_entry->asm_origin, pc_entry->pc};
    default:
      return {};
  }
}

// is there a breakpoint at this $pc?
static bool test_breakpoint(uint64_t pc) {
  return state::breakpoint_pcs.find(pc) != state::breakpoint_pcs.end();
}

// search for the given breakpoint, return index and Breakpoint
static std::optional<std::deque<Breakpoint>::iterator> lookup_breakpoint(Pane type, Location origin) {
  for (auto it = state::breakpoints.begin(); it != state::breakpoints.end(); it++)
    if (it->type == type && it->origin.path() == origin.path() && it->origin.line() == origin.line())
      return it;
  return {};
}

namespace events {
  static bool on_reset() {
    visualiser::processor::cpu.reset_flag();
    return true;
  }

  static bool on_enter(Pane pane) {
    if (pane == Pane::Source) {
      using namespace visualiser::processor;

      if (cpu.is_running()) {
        cpu.clear_debug_messages();
        cpu.step(state::current_cycle);
        update_pc();
        update_debug_lines();
        update_align_pane_pc();
      } else {
        state::is_running = false;
        state::debug_lines.clear();
      }

      return true;
    } else {
      // TODO
      return false;
    }
  }

  static bool on_vert_arrow(PaneStateData* pane, bool is_down) {
    if (!state::show_selected_line) return false;

    state::selected_line += is_down ? 1 : -1;
    update_selected_line();
    update_pane_positions_from_selection();
    return state::selected_line < 0 || state::selected_line >= state::selected_pane->lines.size();
  }

  static bool on_horiz_arrow(PaneStateData* pane, bool is_right) {
    if (!state::show_selected_line) return false;
    state::do_update_selected_line = true;
    return true;
  }

  static bool on_B(PaneStateData* pane) { // toggle breakpoint
    // lookup our current location in this pane
    auto current_location = get_selected_location();
    if (!current_location.has_value()) return false;
    std::cerr << "on_B: " << current_location.value().first.path() << ":" << current_location.value().first.line() << std::endl;

    // check if this breakpoint already exists, toggle it
    auto breakpoint = lookup_breakpoint(pane->type, current_location.value().first);
    if (breakpoint.has_value()) {
      state::breakpoints.erase(breakpoint.value());
      state::breakpoint_pcs.erase(breakpoint.value()->pc);
    } else {
      // insert breakpoint based on selected line
      if (auto selected = get_selected_location(); selected.has_value()) {
        state::breakpoints.push_back({
                                         .type = state::selected_pane->type,
                                         .origin = selected->first,
                                         .pc = selected->second,
                                     });
        state::breakpoint_pcs.insert(selected->second);
      }
    }

    return true;
  }

  static bool on_H() { // toggle IS_RUNNING bit in $flag
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
    return true;
  }

  static bool on_J(PaneStateData* pane) {
    if (state::show_selected_line && !state::source_pane.selected_lines.empty()) {
      if (auto entry = visualiser::assembly::locate_line(state::source_pane.selected_lines.front())) {
        update_pc(entry->pc);
        return true;
      }
    }
    return false;
  }

  static bool on_S() {
    state::show_selected_line = !state::show_selected_line;
    update_selected_line();
    return true;
  }
}// namespace events

// receive an event in this window
static bool on_event(ftxui::Event e) {
  if (e == ftxui::Event::Character('h')) return events::on_H();
  if (e == ftxui::Event::Character('r')) return events::on_reset();
  if (e == ftxui::Event::Character('s')) return events::on_S();
  return false;
}

// receive an event on the given pane
static bool pane_on_event(PaneStateData* pane, ftxui::Event &e) {
  update_selected_pane();

  // we either (a) intercept the event, or (b) pose as middleware to listen to the response
  if (e == ftxui::Event::Return && events::on_enter(pane->type)) return true;
  if (e == ftxui::Event::Character('j') && events::on_J(pane)) return true;
  if (e == ftxui::Event::Character('b') && events::on_B(pane)) return true;

  int old_selected = *pane->pos_ptr;

  // forward event to pane's component
  bool handled = pane->component->OnEvent(e);

  if (auto is_down = e == ftxui::Event::ArrowDown; (is_down || e == ftxui::Event::ArrowUp) && events::on_vert_arrow(pane, is_down)) return true;
  if (auto is_right = e == ftxui::Event::ArrowRight; (is_right || e == ftxui::Event::ArrowLeft) && events::on_horiz_arrow(pane, is_right)) return handled;

  // did the event change the pane's position?
  if (int new_selected = *pane->pos_ptr; new_selected != old_selected) {
    // scroll other panes to match
    int delta = new_selected - old_selected;
    for (PaneStateData* p : state::panes)
      if (pane->type != p->type) *p->pos_ptr += delta;
  }

  return handled;
}

void visualiser::tabs::CodeExecutionTab::init() {
  using namespace ftxui;
  const int paneMaxHeight = 15, debugMaxHeight = 8;

  // ensure $pc fields are all setup correctly
  update_pc(0);

  state::show_selected_line_toggle = create_checkbox("Show Line Selection", state::show_selected_line);
  state::is_running_toggle = create_checkbox("Is Running", [] {
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
  }, state::is_running);

  // pane for source code
  state::source_pane.component = Scroller(Renderer([](bool _) {
    std::vector<Element> elements;
    auto &lines = state::source_pane.lines = assembly::get_file_lines(assembly::source->path);
    elements.reserve(lines.size());
    for (int i = 0; i < lines.size(); i++) {
      auto pc_entry = visualiser::assembly::locate_line(i + 1);
      elements.push_back(test_breakpoint(pc_entry->pc)
        ? hbox(text("◉ ") | color(Color::Red), text(lines[i]))
        : text(lines[i]));
    }

    elements[state::pc_entry->line_no - 1] |= style::highlight_execution;

    if (state::show_selected_line && state::selected_pane) {
      auto style = get_line_highlight_style(Pane::Source);
      for (auto &line: state::source_pane.selected_lines)
        elements[line - 1] |= *style;
    }

    return vbox(std::move(elements));
  }), state::source_pane.pos_ptr) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  // pane for original assembly
  state::asm_pane.component = Scroller(Renderer([](bool f) {
    std::vector<Element> elements;
    auto &lines = state::asm_pane.lines = assembly::get_file_lines(state::pc_entry->asm_origin.path());
    elements.reserve(lines.size());
    for (int i = 0; i < lines.size(); i++) {
      auto pc_entries = visualiser::assembly::locate_asm_line(state::pc_entry->asm_origin.path(), i + 1);
      elements.push_back(!pc_entries.empty() && test_breakpoint(pc_entries.front()->pc)
                         ? hbox(text("◉ ") | color(Color::Red), text(lines[i]))
                         : text(lines[i]));
    }

    elements[state::pc_entry->asm_origin.line() - 1] |= style::highlight_execution;

    if (state::show_selected_line && state::selected_pane) {
      auto style = get_line_highlight_style(Pane::Assembly);
      for (const auto &line: state::asm_pane.selected_lines)
        elements[line - 1] |= *style;
    }

    return vbox(std::move(elements));
  }), state::asm_pane.pos_ptr) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  Component debugComponent = Scroller(Renderer([](bool f) {
    return state::debug_lines.empty() ? text("(None)") : vbox(state::debug_lines);
  })) | size(HEIGHT, LESS_THAN, debugMaxHeight);

  Component layout = Container::Vertical({
    Container::Horizontal({
      state::show_selected_line_toggle,
      state::is_running_toggle
    }),
    Container::Horizontal({
      state::asm_pane.component | CatchEvent([](Event e) {
        return pane_on_event(&state::asm_pane, e);
      }),
      state::source_pane.component | CatchEvent([](Event e) {
        return pane_on_event(&state::source_pane, e);
      })
    }),
    debugComponent
  });

  content_ = Renderer(layout, [debugComponent] {
    // additional information
    std::vector<Element> elements;
    elements.push_back(text("Breakpoints: " + std::to_string(state::breakpoints.size())));

    std::vector<Element> children{text("Status: ")};
    if ((state::is_running = visualiser::processor::cpu.is_running())) {
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

    if (state::do_update_selected_line) {
      if (state::selected_pane) {
        // transition selected line to the new pane
        update_selected_pane();
        state::selected_line = state::selected_pane->selected_lines.empty() ? 0 : state::selected_pane->selected_lines.front();

        update_selected_line();
        update_pane_positions_from_selection();
      }
      state::do_update_selected_line = false;
    }

    return vbox({
      hbox({
        state::show_selected_line_toggle->Render(),
        text(" | "),
        state::is_running_toggle->Render(),
      }),
      hbox({
        vbox({
          text(state::pc_entry->asm_origin.path()) | bold,
          separator(),
          state::asm_pane.component->Render(),
        }) | (state::asm_pane.component->Focused() ? borderDouble : border),
        vbox({
          text("Machine Code") | bold,
          separator(),
          state::source_pane.component->Render(),
        }) | (state::source_pane.component->Focused() ? borderDouble : border)
      }),
      window(text("Debug"), debugComponent->Render(), debugComponent->Focused() ? DOUBLE : LIGHT),
      vbox(elements)
    });
  }) | CatchEvent(on_event);

  help_ = Renderer([&] {
    return create_key_help_pane({
      {"b", "toggle line breakpoint"},
      {"h", "start/stop processor"},
      {"j", "jump to selected line"},
      {"r", "reset the processor's state"},
      {"s", "toggle line select"},
    });
  });
}
