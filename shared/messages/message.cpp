#include <iostream>

#include "message.hpp"
#include "shell.hpp"

namespace message {
    void Message::print_type_suffix(std::ostream &os) {
        switch (m_level) {
            case Level::Note:
                os << ANSI_BLUE "note" ANSI_RESET;
                break;
            case Level::Warning:
                os << ANSI_YELLOW "warning" ANSI_RESET;
                break;
            case Level::Error:
                os << ANSI_RED "error" ANSI_RESET;
                break;
        }

        if (int code = get_code(); code != -1) {
            std::cout << " (" << code << ")";
        }
    }

    void Message::print(std::ostream &os) {
        m_loc.print(os) << ": ";
        print_type_suffix(os);
        os << ": " << m_msg.str() << std::endl;
    }

    Level level_from_int(int level) {
        if (level < 1) return Level::Note;
        if (level == 1) return Level::Warning;
        return Level::Error;
    }
}
