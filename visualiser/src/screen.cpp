#include "screen.hpp"
#include "tabs/execution.hpp"
#include "tabs/registers.hpp"
#include "tabs/tab.hpp"
#include "util.hpp"
#include "tabs/memory.hpp"
#include "tabs/settings.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

static int current_tab = 0;
static ftxui::Component tab_nav, tab_container, tab_help;

ftxui::Element visualiser::tabs::create_key_help_pane(const std::map<std::string, std::string> &keys) {
  using namespace ftxui;
  std::vector<Element> children, names, descriptions;
  static const int rows = 3;

  auto it = keys.begin();
  for (size_t i = 0; i < keys.size(); i += rows) {
    names.clear();
    descriptions.clear();

    for (int j = 0; j < rows && it != keys.end(); j++, it++) {
      names.push_back(text(it->first + ' ') | bold);
      descriptions.push_back(text(it->second));
    }

    children.push_back(vbox(names));
    children.push_back(vbox(descriptions));
    children.push_back(separator());
  }

  // remove last separator()
  children.pop_back();
  return hbox(children);
}

// set current_tab=index & take focus
static bool force_tab_focus(int index) {
  current_tab = index;
  tab_nav->TakeFocus();
  return true;
}

void visualiser::launch() {
  using namespace ftxui;
  using namespace tabs;

  // create tabs
  CodeExecutionTab tab_code_execution;
  RegistersTab tab_registers;
  MemoryTab tab_memory;
  SettingsTab tab_settings;
  std::vector<Tab *> tab_list = {&tab_code_execution, &tab_registers, &tab_memory, &tab_settings};

  // tab navigation buttons
  std::vector<std::string> tab_headers = map(tab_list,
                                             std::function<std::string(Tab *&)>([](auto &t) { return t->title(); }));
  tab_nav = Toggle(&tab_headers, &current_tab);

  // container which selects child based on tab index
  std::vector<Component> tab_contents = map(tab_list,
                                            std::function<Component(Tab *&)>([](auto &t) { return t->content(); }));
  tab_container = Container::Tab(tab_contents, &current_tab);

  std::vector<Component> tab_helps = map(tab_list,
                                            std::function<Component(Tab *&)>([](auto &t) { return t->help(); }));
  tab_help = Container::Tab(tab_helps, &current_tab);

  // create the main UI
  auto container = Container::Vertical({
                                           tab_nav,
                                           tab_container
                                       });

  auto renderer = Renderer(container, [&] {
    return vbox({
      vbox({
          tab_nav->Render(),
          separator(),
          tab_container->Render(),
        }) | border,
        tab_help->Render() | border,
    });
  }) | CatchEvent([&](Event e) {
    if (e == Event::F1) return force_tab_focus(0);
    if (e == Event::F2) return force_tab_focus(1);
    if (e == Event::F3) return force_tab_focus(2);
    return false;
  });

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(renderer);
}
