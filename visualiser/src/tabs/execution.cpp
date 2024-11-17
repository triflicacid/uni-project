#include <regex>
#include "execution.hpp"
#include "tabs.hpp"
#include "assembly.hpp"
#include "util.hpp"
#include "processor.hpp"
#include "screen.hpp"

std::string visualiser::tabs::execution::title = "Main";
int visualiser::tabs::execution::index = 0;
ftxui::Component visualiser::tabs::execution::content = nullptr;

static uint64_t current_pc = 0;
static int current_cycle = 0;
static ftxui::Decorator highlight_line;

static std::string normalise_newlines(const std::string& input) {
    return std::regex_replace(input, std::regex("\r\n"), "\n");
}

static std::string strip_ansi(const std::string& input) {
    return std::regex_replace(input, std::regex("\e\\[[0-9;]*[a-zA-Z]"), "");
}

void visualiser::tabs::execution::init() {
    using namespace ftxui;
    highlight_line = bgcolor(Color::Yellow) | color(Color::Black);

    content = Renderer([&] {
        // get source of $pc
        auto opt_pc = assembly::locate_pc(current_pc);

        // pane - view source file
        std::vector<Element> elements;
        if (opt_pc.has_value()) {
            auto &op = opt_pc.value();

            auto lines = assembly::get_file_lines(op.loc.path());
            for (int i = 0; i < lines.size(); i++) {
                // create node for this line (highlight if needed)
                auto node = text(lines[i]);
                if (i + 1 == op.loc.line()) node |= highlight_line;
                elements.push_back(node);
            }
        } else {
            elements.push_back(text("$pc cannot be traced to source") | color(Color::RedLight));
        }

        auto source_pane = vbox({
            (opt_pc.has_value() ? text(opt_pc.value().loc.path()) : text("")) | bold,
            vbox(std::move(elements)) | frame | border,
        });

        // pane - view assembly source
        elements.clear();
        auto lines = assembly::get_file_lines(assembly::source->path);
        for (int i = 0; i < lines.size(); i++) {
            // create node for this line (highlight if needed)
            auto node = text(lines[i]);
            if (opt_pc.has_value() && i + 1 == opt_pc.value().line_no) node |= highlight_line;
            elements.push_back(node);
        }

        auto asm_pane = vbox({
                text("Compiled Assembly") | bold,
                vbox(std::move(elements)) | frame | border,
        });

        // pane - information
        elements.clear();
        if (::processor::debug::any()) {
            std::string debug = processor::debug_stream.str();
            debug = strip_ansi(normalise_newlines(debug));

            std::vector<Element> debug_lines;
            std::istringstream stream(debug);
            std::string line;
            while (std::getline(stream, line)) debug_lines.push_back(text(line));

            if (!debug_lines.empty()) elements.push_back(vbox(debug_lines) | border);
        }
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
                 text(std::to_string(current_cycle)) | color(Color::RedLight),
        }));
        elements.push_back(hbox({
                 text("$pc: "),
                 text("0x" + to_hex_string(current_pc)) | color(Color::RedLight),
        }));

        auto info_pane = vbox(elements);

        // return pane combination
        return vbox({
            hbox({
                source_pane,
                separator(),
                asm_pane
            }),
            separator(),
            info_pane
        });
    });
}

static void on_reset() {
    visualiser::processor::cpu.reset_flag();
}

static void on_enter() {
    using namespace visualiser::processor;

    if (cpu.is_running()) {
        cpu.step(current_cycle);
        current_pc = cpu.read_pc();
    }
}

bool visualiser::tabs::execution::on_event(ftxui::Event &event) {
    using namespace ftxui;

    if (event == Event::Return) {
        on_enter();
        return true;
    } else if (event == Event::Special({18})) {
        on_reset();
        return true;
    }

    return false;
}
