#include "screen.hpp"
#include "tabs/execution.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

ftxui::Component visualiser::main;
ftxui::ScreenInteractive visualiser::screen = ftxui::ScreenInteractive::Fullscreen();

// our current tab
static int current_tab = 0;

void visualiser::init() {
    using namespace ftxui;
    using namespace tabs;

    // initialise each tab
    execution::init();

    // create navigation bar
    auto tab_nav = Container::Horizontal({
        Button(execution::title, [&] { current_tab = execution::index; }),
    });

    // create tab content
    auto tab_content = Container::Tab({
        execution::content,
    }, &current_tab);

    // create the main UI
    auto body = Container::Vertical({
        tab_nav,
        tab_content
    });

    main = CatchEvent(body, [&](Event event) {
        if (current_tab == execution::index) {
            return execution::on_event(event);
        }

        // what?
        return false;
    });
}

void visualiser::display() {
    visualiser::screen.Loop(visualiser::main);
}
