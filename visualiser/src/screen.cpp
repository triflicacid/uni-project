#include "screen.hpp"
#include "tabs/execution.hpp"
#include "tabs/registers.hpp"
#include "tabs/tab.hpp"
#include "util.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

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
  int current_tab = 0;
  auto tab_nav = Toggle(&tab_headers, &current_tab);

  // container which selects child based on tab index
  std::vector<Component> tab_contents = map(tab_list,
                                            std::function<Component(Tab *&)>([](auto &t) { return t->content(); }));
  auto tab_container = Container::Tab(tab_contents, &current_tab);

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
  });

  auto screen = ScreenInteractive::TerminalOutput();
  screen.Loop(renderer);
}
