#include "screen.hpp"
#include "tabs/execution.hpp"
#include "tabs/registers.hpp"
#include "tabs/tab.hpp"
#include "util.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

static int current_tab = 0;
static ftxui::Component tab_nav, tab_container;

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
  std::vector<Tab *> tab_list = {&tab_code_execution, &tab_registers};

  // tab navigation buttons
  std::vector<std::string> tab_headers = map(tab_list,
                                             std::function<std::string(Tab *&)>([](auto &t) { return t->title(); }));
  tab_nav = Toggle(&tab_headers, &current_tab);

  // container which selects child based on tab index
  std::vector<Component> tab_contents = map(tab_list,
                                            std::function<Component(Tab *&)>([](auto &t) { return t->content(); }));
  tab_container = Container::Tab(tab_contents, &current_tab);

  // create the main UI
  auto container = Container::Vertical({
                                           tab_nav,
                                           tab_container
                                       });

  auto renderer = Renderer(container, [&] {
    return vbox({
                    tab_nav->Render(),
                    separator(),
                    tab_container->Render(),
                });
  }) | CatchEvent([&](Event e) {
    if (e == Event::F1) return force_tab_focus(0);
    if (e == Event::F2) return force_tab_focus(1);
    return false;
  });

  auto screen = ScreenInteractive::TerminalOutput();
  screen.Loop(renderer);
}
