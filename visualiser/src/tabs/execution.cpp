#include "execution.hpp"
#include "sources.hpp"
#include "processor.hpp"
#include "style.hpp"
#include "util.hpp"
#include "components/scroller.hpp"
#include "components/checkbox.hpp"
#include <ftxui/component/event.hpp>
#include <unordered_set>

struct PaneStateData {
  visualiser::sources::Type type;
  ftxui::Component component;
  const visualiser::sources::File* file; // file which we are viewing
  int *pos_ptr; // pointer top ScrollerBase::selected_
  std::unordered_set<int> selected_lines; // store selected lines in each pane

  PaneStateData(visualiser::sources::Type type) : type(type) {}
};

namespace state {
  static int current_cycle = 0;                            // processor's current cycle
  static std::vector<ftxui::Element> debug_lines; // contains lines in the debug field - preserves values

  static bool show_selected_line = false; // show light-blue selected line(s)?
  static ftxui::Component show_selected_line_toggle; // toggle for show_selected_line Boolean
  static bool is_running = false;
  static ftxui::Component is_running_toggle; // toggle for IS_RUNNING
  static bool do_update_selected_line = false;

  static PaneStateData source_pane(visualiser::sources::Type::Source);
  static PaneStateData asm_pane(visualiser::sources::Type::Assembly);
  static PaneStateData lang_pane(visualiser::sources::Type::Language);
  static std::array<PaneStateData*, 3> panes{&source_pane, &asm_pane, &lang_pane};
  static PaneStateData* selected_pane = nullptr;
  static int selected_line = 0; // current line which is selected
}// namespace state

// get pane state given pane type
static PaneStateData& get_pane_by_type(visualiser::sources::Type type) {
  switch (type) {
    case visualiser::sources::Type::Source:
      return state::source_pane;
    case visualiser::sources::Type::Assembly:
      return state::asm_pane;
    case visualiser::sources::Type::Language:
      return state::lang_pane;
  }
}

// update panes' positions based on state::current_pc
static void update_align_pane_pc() {
  if (!visualiser::processor::pc_line) return;

  // TODO update
  // align source pane
  *state::source_pane.pos_ptr = visualiser::processor::pc_line->line_no;

  // align assembly pane
  *state::asm_pane.pos_ptr = visualiser::processor::pc_line->asm_origin.line();

  // align language (Edel) pane
  if (auto& origin = visualiser::processor::pc_line->lang_origin) {
    *state::lang_pane.pos_ptr = origin->line();
  }
}

// update state::selected_pane
static void update_selected_pane() {
  state::selected_pane =
    state::source_pane.component->Focused() ? &state::source_pane
  : state::asm_pane.component->Focused() ? &state::asm_pane
  : state::lang_pane.component->Focused() ? &state::lang_pane
  : nullptr;
}

// get the first (minimum) selected line
static unsigned int get_first_line_selected() {
  if (!state::selected_pane) return 0;
  auto& lines = state::selected_pane->selected_lines;
  return lines.empty()
    ? 0
    : *std::min_element(lines.begin(), lines.end());
}

// update selected line information - trace lines from current pane to others
static void update_selected_line() {
  using namespace state;
  update_selected_pane();
  if (!show_selected_line || !selected_pane) return;

  // ensure selected line is in bounds
  clamp(selected_line, 0, int(selected_pane->file->lines.size()));

  // clear selection
  for (PaneStateData* pane : state::panes)
    pane->selected_lines.clear();

  // select the current line
  selected_pane->selected_lines.insert(selected_line);

  if (selected_pane->type == visualiser::sources::Type::Source) {
    auto location = visualiser::sources::locate_line(selected_line);
    if (location) {
      asm_pane.selected_lines.insert(location->asm_origin.line());
      if (location->lang_origin) lang_pane.selected_lines.insert(location->lang_origin->line());
    }
  } else if (selected_pane->type == visualiser::sources::Type::Assembly) {
//    auto key = std::make_pair(visualiser::processor::pc_line->asm_origin.path(), selected_line);
//    visualiser::sources::trace.bfs(key, [](const auto& trace) {
//      visualiser::sources::FileLine* file_line = visualiser::sources::trace.get(trace)->get();
//      PaneStateData& pane = get_pane_by_type(file_line->parent->type);
//      pane.selected_lines.insert(trace.second);
//    });

    auto locations = visualiser::sources::locate_asm_line(visualiser::processor::pc_line->asm_origin.path(), selected_line);
    for (auto &location: locations) {
      source_pane.selected_lines.insert(location->line_no);
      if (location->lang_origin) lang_pane.selected_lines.insert(location->lang_origin->line());
    }
  } else if (selected_pane->type == visualiser::sources::Type::Language) {
    if (auto& origin = visualiser::processor::pc_line->lang_origin) {
      auto locations = visualiser::sources::locate_lang_line(origin->path(), selected_line);
      for (auto& location: locations) {
        source_pane.selected_lines.insert(location->line_no);
        asm_pane.selected_lines.insert(location->asm_origin.line());
      }
    }
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
static ftxui::Decorator* get_line_highlight_style(visualiser::sources::Type pane) {
  return state::selected_pane && pane == state::selected_pane->type ? &visualiser::style::highlight_selected : &visualiser::style::highlight_traced;
}

// get PC's Location
static Location get_pc_location_in_pane(const visualiser::sources::PCLine* pc_entry, visualiser::sources::Type pane) {
  switch (pane) {
    case visualiser::sources::Type::Assembly:
      return pc_entry->asm_origin;
    case visualiser::sources::Type::Language:
      if (pc_entry->lang_origin)
        return pc_entry->lang_origin.value();
    default:
      return Location(visualiser::sources::s_source->path, pc_entry->line_no);
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
      std::string msg = message->message.str();
      if (msg.empty()) {
        os << "instruction: " << message->instruction;
      } else {
        os << message->instruction << ": " << message->message.str();
      }
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

// execute a single step of the processor, perform necessary state:: updates and mutations
static void step_processor() {
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
}

namespace events {
  static bool on_reset() {
    visualiser::processor::cpu.reset_flag();
    return true;
  }

  // execute *one* cycle
  static bool on_space(visualiser::sources::Type pane) {
    if (!visualiser::processor::pc_line) return false;

    switch (pane) {
      case visualiser::sources::Type::Source:
        // viewing machine code, step processor once
        step_processor();
        return true;
      case visualiser::sources::Type::Assembly: {
        // step beyond current line
        int source_line_count = state::asm_pane.file->lines[visualiser::processor::pc_line->asm_origin.line()].pc_trace.size();
        for (int i = 0; i < source_line_count; i++)
          step_processor();
        return true;
      }
      case visualiser::sources::Type::Language: {
        // step beyond current line
        auto& origin = visualiser::processor::pc_line->lang_origin;
        if (origin) {
          auto& traces = state::lang_pane.file->lines[origin->line()].pc_trace;
          if (traces.empty()) {
            step_processor();
            break;
          }

          // determine number of lines to execute
          int step_count = 0;
          uint64_t pc = traces.front()->pc;
          for (auto& trace : traces) {
            if (trace->pc == pc) {
              step_count++;
              pc += 8;
            }
          }

          std::cerr << "space on lang pane: execute " << step_count << " steps" << std::endl;
          for (int i = 0; i < step_count; i++)
            step_processor();
        } else {
          step_processor();
        }
        return true;
      }

      default: ;
    }

    return false;
  }

  // execute until next breakpoint or done
  static bool on_enter(visualiser::sources::Type pane) {
    do {
      step_processor();
    } while (visualiser::processor::cpu.is_running() && !visualiser::processor::pc_line->has_breakpoint());

    return true;
  }

  static bool on_vert_arrow(PaneStateData* pane, bool is_down) {
    if (!state::show_selected_line) return false;

    state::selected_line += is_down ? 1 : -1;
    update_selected_line();
    update_pane_positions_from_selection();
    return state::selected_line < 0 || state::selected_line >= state::selected_pane->file->lines.size();
  }

  static bool on_horiz_arrow(PaneStateData* pane, bool is_right) {
    if (!state::show_selected_line) return false;
    state::do_update_selected_line = true;
    return true;
  }

  static bool on_B(PaneStateData* pane) {
    // lookup our current location in this pane and toggle breakpoint
    auto &lines = state::selected_pane->file->lines[state::selected_line].pc_trace;
    if (lines.empty()) return false;

    lines.front()->toggle_breakpoint();
    return true;
  }

  static bool on_H() { // toggle IS_RUNNING bit in $flag
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
    return true;
  }

  static bool on_J(PaneStateData* pane) {
    if (state::show_selected_line && !state::source_pane.selected_lines.empty()) {
      if (auto entry = visualiser::sources::locate_line(get_first_line_selected())) {
        visualiser::processor::update_pc(entry->pc);
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
  if (e == ftxui::Event::h) return events::on_H();
  if (e == ftxui::Event::r) return events::on_reset();
  if (e == ftxui::Event::s) return events::on_S();
  return false;
}

// receive an event on the given pane
static bool pane_on_event(PaneStateData* pane, ftxui::Event &e) {
  update_selected_pane();

  // we either (a) intercept the event, or (b) pose as middleware to listen to the response
  if (e == ftxui::Event::Character(" ") && events::on_space(pane->type)) return true;
  if (e == ftxui::Event::Return && events::on_enter(pane->type)) return true;
  if (e == ftxui::Event::j && events::on_J(pane)) return true;
  if (e == ftxui::Event::b && events::on_B(pane)) return true;

  int old_selected = *pane->pos_ptr;

  // forward event to pane's component
  const bool handled = pane->component->OnEvent(e);

  if (auto is_down = e == ftxui::Event::ArrowDown; (is_down || e == ftxui::Event::ArrowUp) && events::on_vert_arrow(pane, is_down)) return true;
  if (auto is_right = e == ftxui::Event::ArrowRight; (is_right || e == ftxui::Event::ArrowLeft) && events::on_horiz_arrow(pane, is_right)) return handled;
  if (e == ftxui::Event::Home || e == ftxui::Event::End) {
    if (state::selected_pane && state::show_selected_line) {
      uint64_t target = e == ftxui::Event::Home
          ? 0
          : state::selected_pane->file->lines.size() - 1;
      if (state::selected_line != target) {
        state::selected_line = target;
        update_selected_line();
        update_pane_positions_from_selection();
      }
    }
    return true;
  }

  // did the event change the pane's position?
  if (int new_selected = *pane->pos_ptr; new_selected != old_selected) {
    // scroll other panes to match
    int delta = new_selected - old_selected;
    for (PaneStateData* p : state::panes)
      if (pane->type != p->type) *p->pos_ptr += delta;
  }

  return handled;
}

// return Element wrapping the current line
static ftxui::Element wrap_line(const visualiser::sources::FileLine &line) {
  // test if there is a breakpoint on this line
  if (line.has_breakpoint()) {
    return hbox(visualiser::style::breakpoint_prefix(), ftxui::text(line.line));
  }

  return ftxui::text(line.line);
}

// create a pane Scroller()
static ftxui::Component create_pane_component(PaneStateData& pane) {
  using namespace ftxui;

  return Scroller(Renderer([&pane](bool f) {
    using namespace visualiser;
    std::vector<Element> elements;

    if (visualiser::processor::pc_line) {
      Location location = get_pc_location_in_pane(visualiser::processor::pc_line, pane.type);
      pane.file = sources::get_file(location.path());
      elements.reserve(pane.file->lines.size());

      // create Elements for each line
      for (auto& line: pane.file->lines) {
        elements.push_back(wrap_line(line));
      }

      // if enabled, show selected/traced lines
      if (state::show_selected_line && state::selected_pane) {
        auto style = get_line_highlight_style(pane.type);
        for (const auto& line: pane.selected_lines) {
          elements[line] |= *style;
        }
      }

      // highlight the $pc line, unless highlighted already
      if (location.line() < elements.size() && !(state::show_selected_line && state::selected_pane && pane.selected_lines.contains(location.line())))
        elements[location.line()] |= style::highlight_execution;
    } else {
      elements.push_back(text("unable to trace source") | color(Color::Red));
    }

    return vbox(std::move(elements));
  }), pane.pos_ptr);
}

void visualiser::tabs::CodeExecutionTab::init() {
  using namespace ftxui;
  const int paneMaxHeight = 16, debugMaxHeight = 10;

  state::show_selected_line_toggle = create_checkbox("Show Line Selection", state::show_selected_line);
  state::is_running_toggle = create_checkbox("Is Running", [] {
    visualiser::processor::cpu.flag_toggle(constants::flag::is_running, true);
  }, state::is_running);

  // pane for source code, .s
  state::source_pane.component = create_pane_component(state::source_pane) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  // pane for assembly code, .asm
  state::asm_pane.component = create_pane_component(state::asm_pane) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  // pane for Edel code, .edel
  state::lang_pane.component = create_pane_component(state::lang_pane) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  Component debugComponent = Scroller(Renderer([](bool f) {
    return state::debug_lines.empty() ? text("(None)") : vbox(state::debug_lines);
  })) | size(HEIGHT, LESS_THAN, debugMaxHeight);

  Component layout = Container::Vertical({
    Container::Horizontal({
      state::show_selected_line_toggle,
      state::is_running_toggle
    }),
    Container::Horizontal({
      state::lang_pane.component | CatchEvent([](Event e) {
        return pane_on_event(&state::lang_pane, e);
      }),
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

    // lookup the processor's status
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
                                text("0x" + to_hex_string(visualiser::processor::pc, 8)) | style::reg,
                            }));

    // flag: update the selected line? (set on arrow keypress)
    if (state::do_update_selected_line) {
      if (state::selected_pane) {
        // transition selected line to the new pane
        update_selected_pane();
        state::selected_line = get_first_line_selected();

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
          text("Edel") | bold,
          separator(),
          text("// " + (visualiser::processor::pc_line && visualiser::processor::pc_line->lang_origin ? std::string(visualiser::processor::pc_line->lang_origin->path()) : "(unknown)")),
          state::lang_pane.component->Render(),
        }) | (state::lang_pane.component->Focused() ? borderDouble : border),
        vbox({
          text("Assembly") | bold,
          separator(),
          text("; " + (visualiser::processor::pc_line ? std::string(visualiser::processor::pc_line->asm_origin.path()) : "(unknown)")),
          state::asm_pane.component->Render(),
        }) | (state::asm_pane.component->Focused() ? borderDouble : border),
        vbox({
          text("Reconstructed Assembly") | bold,
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
      {"Space", "execute current line"},
      {"Return", "start execution"}
    });
  });
}
