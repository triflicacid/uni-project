#include <ftxui/component/event.hpp>
#include "sources.hpp"
#include "processor.hpp"
#include "util.hpp"
#include "style.hpp"
#include "components/scroller.hpp"
#include "components/checkbox.hpp"

namespace state {
  static ftxui::Component menu_focus_component; // if focused, highlight selected file
  static std::vector<const visualiser::File*> files;
  static int menu_selected; // Menu::selected_
  static ftxui::Component file_pane;
  static bool show_selected = false;
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
inline const visualiser::File* selected() {
  return state::files[state::menu_selected];
}

// given file, generate element
static ftxui::Element generate_file_element(const visualiser::File& file) {
  ftxui::Element base = ftxui::text(file.path.string());
  return state::menu_focus_component->Focused() && selected() == &file
    ? base | visualiser::style::highlight
    : base;
}

// return Element wrapping the current line
static ftxui::Element wrap_line(const visualiser::FileLine &line) {
  // test if there is a breakpoint on this line
  if (visualiser::line_has_breakpoint(line)) {
    return hbox(visualiser::style::breakpoint_prefix(), ftxui::text(line.line));
  }

  return ftxui::text(line.line);
}

// ensure state::selected_line is in bounds
static void validate_selected_line() {
  const visualiser::File* file = selected();
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
    const visualiser::File* file = selected();
    auto& line = file->lines[state::selected_line - 1];
    if (line.trace.empty()) return true;

    auto& pc_line = line.trace.front();

    // trace back to previous abstraction level
    switch (file->type) {
      case visualiser::Type::Language: // -> assembly
        state::menu_selected = find_file(pc_line->asm_origin.path());
        state::selected_line = pc_line->asm_origin.line();
        break;
      case visualiser::Type::Assembly: // -> source
        state::menu_selected = 0;
        state::selected_line = pc_line->line_no;
        break;
      default: ;
    }

    return true;
  }

  static bool on_left_bracket(ftxui::Event& e) { // ']'
    const visualiser::File* file = selected();
    const visualiser::FileLine& file_line = file->lines[state::selected_line - 1];
    std::cerr << "Traced " << file_line.trace.size() << " lines" << std::endl;
    if (file_line.trace.empty()) return true;

    const visualiser::PCLine* pc_line = file_line.trace.front();
    const Location* location = nullptr;
    switch (file->type) {
      case visualiser::Type::Source: // -> assembly
        location = &pc_line->asm_origin;
        break;
      case visualiser::Type::Assembly: // -> language
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
    auto& line = selected()->lines[state::selected_line - 1];
    visualiser::processor::toggle_breakpoint(line.trace.front());
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
  if (e == ftxui::Event::s) {
    state::show_selected = !state::show_selected;
    return true;
  }

  return false;
}

void visualiser::tabs::SourcesTab::init() {
  using namespace ftxui;
  const int paneMaxHeight = 25;

  // toggle for show_selected
  Component show_selected_toggle = create_checkbox("Show Line Selection", state::show_selected);

  // create menu containing file names
  std::vector<std::string> file_names;
  state::files.reserve(visualiser::files.size());
  file_names.reserve(visualiser::files.size());
  for (auto& [path, file] : visualiser::files) {
    state::files.push_back(&file);
    file_names.push_back(path.string());
  }

  // create pane which will contain selected file's content
  state::file_pane = Scroller(Renderer([](bool f) {
    // create Elements for each line of the file
    const File* file = selected();
    std::vector<Element> elements;
    elements.reserve(file->lines.size());
    for (auto& line : file->lines) {
      elements.push_back(wrap_line(line));
    }

    // highlight the selected line
    if (state::show_selected) {
      elements[state::selected_line - 1] |= style::highlight_selected;
    }

    return vbox(elements);
  })) | size(HEIGHT, LESS_THAN, paneMaxHeight) | CatchEvent(file_pane_on_event);

  Component layout = Container::Vertical({
    show_selected_toggle,
    state::menu_focus_component = Container::Horizontal({
        Menu({
          .entries = file_names,
          .selected = &state::menu_selected,
          .on_change = on_change_file
        }),
        state::file_pane
      })
  });

  content_ = Renderer(layout, [show_selected_toggle] {
    std::vector<Element> elements;

    // sort source files into correct lists
    const File* source_file = get_file(source->path);
    std::vector<Element> asm_files, lang_files;
    for (auto& [path, file] : visualiser::files) {
      if (file.type == visualiser::Type::Assembly) {
        asm_files.push_back(generate_file_element(file));
      } else if (file.type == Type::Language) {
        lang_files.push_back(generate_file_element(file));
      }
    }

    return hbox({
      vbox({
              show_selected_toggle->Render(),
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
      {"s", "toggle line select"},
      {"[", "trace line backwards"},
      {"]", "trace line forwards"},
    });
  });
}