#include "sources.hpp"
#include "processor.hpp"
#include "util.hpp"
#include "style.hpp"
#include "components/scroller.hpp"

namespace state {
  ftxui::Component content;
  std::vector<const visualiser::File*> files;
  int menu_selected;
}

// get the selected file
inline const visualiser::File* selected() {
  return state::files[state::menu_selected];
}

// given file, generate element
static ftxui::Element generate_file_element(const visualiser::File& file) {
  ftxui::Element base = ftxui::text(file.path.string());
  return state::content->Focused() && selected() == &file
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

void visualiser::tabs::SourcesTab::init() {
  using namespace ftxui;
  const int paneMaxHeight = 20;

  // create menu containing file names
  std::vector<std::string> file_names;
  state::files.reserve(visualiser::files.size());
  file_names.reserve(visualiser::files.size());
  for (auto& [path, file] : visualiser::files) {
    state::files.push_back(&file);
    file_names.push_back(path.string());
  }

  Component file_pane = Scroller(Renderer([](bool f) {
    const File* file = selected();

    // create Elements for each line of the file
    std::vector<Element> elements;
    elements.reserve(file->lines.size());
    for (auto& line : file->lines) {
      elements.push_back(wrap_line(line));
    }

    return vbox(elements);
  })) | size(HEIGHT, LESS_THAN, paneMaxHeight);

  Component layout = Container::Horizontal({
    Menu({
      .entries = file_names,
      .selected = &state::menu_selected
    }),
    file_pane
  });

  content_ = state::content = Renderer(layout, [file_pane] {
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
              window(
                  text("Machine Code"),
                  asm_files.empty() ? text("(None)") : vbox(generate_file_element(*source_file)),
                  LIGHT
              ),
              window(
                  text("Assembly Sources"),
                  vbox(asm_files),
                  LIGHT
              ),
              window(
                  text("High-Level Sources"),
                  lang_files.empty() ? text("(None)") : vbox(lang_files),
                  LIGHT
              ),
          }),
      state::content->Focused()
        ? vbox({
                   text(selected()->path) | center | bold,
                   separator(),
                   file_pane->Render()
               }) | (file_pane->Focused() ? borderDouble : border)
        : vbox(text("No file selected")) | border
    });
  });
}