#include <ftxui/component/event.hpp>
#include "sources.hpp"
#include "processor.hpp"
#include "util.hpp"
#include "style.hpp"
#include "components/scroller.hpp"

namespace state {
  static ftxui::Component menu_focus_component; // if focused, highlight selected file
  static std::vector<const visualiser::sources::File*> files;
  static int menu_selected; // Menu::selected_
  static ftxui::Component file_pane;
  static int selected_line = 1; // 1-indexed
}

// return index of given file
static int find_file(const std::filesystem::path& path) {
  for (int i = 0; i < state::files.size(); i++) {
    if (state::files[i]->path == path) {
      return i;
    }
  }

  return -1;
}

// get the selected file
inline const visualiser::sources::File* selected() {
  return state::files[state::menu_selected];
}

// get the selected file's line
inline const visualiser::sources::FileLine& selected_line() {
  return selected()->lines[state::selected_line - 1];
}

// given file, generate element
static ftxui::Element generate_file_element(const visualiser::sources::File& file) {
  ftxui::Element base = ftxui::text(file.path.string());

  if (int breakpoints = file.count_breakpoints()) {
    base = ftxui::hbox({
     ftxui::text("(" + (breakpoints == 1 ? "" : std::to_string(breakpoints) + "×")),
     ftxui::text(visualiser::style::breakpoint_icon) | visualiser::style::breakpoint_colour,
     ftxui::text(") "),
     base
    });
  }

  return state::menu_focus_component->Focused() && selected() == &file
    ? base | visualiser::style::highlight
    : base;
}

// return Element wrapping the current line
static ftxui::Element wrap_line(const visualiser::sources::FileLine &line) {
  // test if there is a breakpoint on this line
  if (line.has_breakpoint()) {
    return hbox(visualiser::style::breakpoint_prefix(), ftxui::text(line.line));
  }

  return ftxui::text(line.line);
}

// ensure state::selected_line is in bounds
static void validate_selected_line() {
  const visualiser::sources::File* file = selected();
  if (!file) return;

  clamp(state::selected_line, 1, (int)file->lines.size() + 1);
}

// update file selection in the menu
static void on_change_file() {
  validate_selected_line();
}

namespace events {
  static bool on_arrow_up(ftxui::Event& e) {
    state::selected_line--;
    if (state::selected_line < 1) state::selected_line = 1;
    return false;
  }

  static bool on_arrow_down(ftxui::Event& e) {
    state::selected_line++;
    size_t limit = selected()->lines.size();
    if (state::selected_line > limit) state::selected_line = limit;
    return false;
  }

  static bool on_home(ftxui::Event& e) {
    state::selected_line = 1;
    return false;
  }

  static bool on_end(ftxui::Event& e) {
    state::selected_line = selected()->lines.size();
    return false;
  }

  static bool on_right_bracket(ftxui::Event& e) { // ']'
    // trace the current line
    const visualiser::sources::File* file = selected();
    auto& line = selected_line();
    if (line.trace.empty()) return true;

    auto& pc_line = line.trace.front();

    // trace back to previous abstraction level
    switch (file->type) {
      case visualiser::sources::Type::Language: // -> assembly
        state::menu_selected = find_file(pc_line->asm_origin.path());
        state::selected_line = pc_line->asm_origin.line();
        break;
      case visualiser::sources::Type::Assembly: // -> source
        state::menu_selected = 0;
        state::selected_line = pc_line->line_no;
        break;
      default: ;
    }

    return true;
  }

  static bool on_left_bracket(ftxui::Event& e) { // ']'
    const visualiser::sources::File* file = selected();
    const visualiser::sources::FileLine& file_line = selected_line();
    if (file_line.trace.empty()) return true;

    const visualiser::sources::PCLine* pc_line = file_line.trace.front();
    const Location* location = nullptr;
    switch (file->type) {
      case visualiser::sources::Type::Source: // -> assembly
        location = &pc_line->asm_origin;
        break;
      case visualiser::sources::Type::Assembly: // -> language
        location = &pc_line->lang_origin;
        break;
      default: ;
    }

    if (location) {
      // make sure that the file actually exists
      if (int file_index = find_file(location->path()); file_index != -1) {
        state::selected_line = location->line();
        state::menu_selected = file_index;
      }
    }

    return true;
  }

  static bool on_b(ftxui::Event& e) {
    auto& line = selected_line();
    if (!line.trace.empty()) line.trace.front()->toggle_breakpoint();
    return true;
  }

  static bool on_e(ftxui::Event& e) {
    // lookup line associated to $pc, jump to its origin (in source)
    if (auto* pc = visualiser::sources::locate_pc(visualiser::processor::pc)) {
      state::selected_line = pc->line_no;
      state::menu_selected = 0; // first file (i.e., source)
      state::file_pane->TakeFocus();
    }

    return true;
  }

  static bool on_j(ftxui::Event& e) {
    const visualiser::sources::FileLine& line = selected_line();
    if (std::optional<uint64_t> pc = line.pc(); pc.has_value() && pc.value() != visualiser::processor::pc)
      visualiser::processor::update_pc(pc.value());
    return true;
  }
}

static bool file_pane_on_event(ftxui::Event e) {
  if (e == ftxui::Event::ArrowUp) return events::on_arrow_up(e);
  if (e == ftxui::Event::Home) return events::on_home(e);
  if (e == ftxui::Event::ArrowDown) return events::on_arrow_down(e);
  if (e == ftxui::Event::End) return events::on_end(e);
  if (e == ftxui::Event::b) return events::on_b(e);
  if (e == ftxui::Event::Character("]")) return events::on_right_bracket(e);
  if (e == ftxui::Event::Character("[")) return events::on_left_bracket(e);
  return false;
}

static bool on_event(ftxui::Event e) {
  if (e == ftxui::Event::e) return events::on_e(e);
  if (e == ftxui::Event::j) return events::on_j(e);
  return false;
}

void visualiser::tabs::SourcesTab::init() {
  using namespace ftxui;
  const int paneMaxHeight = 25;

  // create menu containing file names
  std::vector<std::string> file_names;
  state::files.reserve(visualiser::sources::files.size());
  file_names.reserve(visualiser::sources::files.size());
  for (auto& [path, file] : visualiser::sources::files) {
    state::files.push_back(&file);
    file_names.push_back(path.string());
  }

  // create pane which will contain selected file's content
  state::file_pane = Scroller(Renderer([](bool focused) {
    // create Elements for each line of the file
    const sources::File* file = selected();
    std::vector<Element> elements;
    elements.reserve(file->lines.size());
    for (auto& line : file->lines) {
      elements.push_back(wrap_line(line));

      // is line being executed? (and not selected)
      if ((!focused || line.n != state::selected_line) && line.contains_pc(processor::pc)) {
        elements.back() |= style::highlight_execution;
      }
    }

    // highlight the selected line
    if (focused) {
      elements[state::selected_line - 1] |= style::highlight_selected;
    }

    return vbox(elements);
  })) | size(HEIGHT, LESS_THAN, paneMaxHeight) | CatchEvent(file_pane_on_event);

  Component layout = Container::Vertical({
    state::menu_focus_component = Container::Horizontal({
        Menu({
          .entries = file_names,
          .selected = &state::menu_selected,
          .on_change = on_change_file
        }),
        state::file_pane
      })
  });

  content_ = Renderer(layout, [] {
    std::vector<Element> elements;

    // sort source files into correct lists
    const sources::File* source_file = sources::get_file(sources::s_source->path);
    std::vector<Element> asm_files, lang_files;
    for (auto& [path, file] : visualiser::sources::files) {
      if (file.type == sources::Type::Assembly) {
        asm_files.push_back(generate_file_element(file));
      } else if (file.type == sources::Type::Language) {
        lang_files.push_back(generate_file_element(file));
      }
    }

    return hbox({
      vbox({
              window(
                  text("Machine Code"),
                  vbox(generate_file_element(*source_file)),
                  LIGHT
              ),
              window(
                  text("Assembly Sources"),
                  asm_files.empty() ? text("(None)") : vbox(asm_files),
                  LIGHT
              ),
              window(
                  text("High-Level Sources"),
                  lang_files.empty() ? text("(None)") : vbox(lang_files),
                  LIGHT
              ),
          }),
      state::menu_focus_component->Focused()
        ? vbox({
                   text(selected()->path) | center | bold,
                   separator(),
                   state::file_pane->Render()
               }) | (state::file_pane->Focused() ? borderDouble : border)
        : vbox(text("No file selected")) | border
    });
  }) | CatchEvent(on_event);

  help_ = Renderer([] {
    return create_key_help_pane({
      {"b", "toggle line breakpoint"},
      {"e", "jump to $pc"},
      {"j", "set $pc to current line"},
      {"[", "trace line backwards"},
      {"]", "trace line forwards"},
    });
  });
}