#include "MessageWithSource.hpp"
#include "util.h"
#include <string>
#include <iostream>
#include <utility>

namespace message {
    MessageWithSource::MessageWithSource(Level level, std::filesystem::path filename, int line, int col, int idx, int len, const std::string& src)
    : Message(level, std::move(filename), line, col) {
        m_idx = std::max(0, idx);
        m_len = std::max(1, std::min(len, (int) src.length() - m_idx));
        m_src = src;
    }

    void MessageWithSource::print_notice() {
        std::cout << ANSI_BLUE "note" ANSI_RESET " " << m_file.string() << ':' << m_line + 1 << ':' << m_col + 1 << ": " << m_msg
                  << '\n' << m_line + 1 << " | " << m_src.substr(0, m_idx) << ANSI_BLUE << m_src.substr(m_idx, m_len) << ANSI_RESET << m_src.substr(m_idx + m_len)
                  << '\n' << std::string(std::to_string(m_line + 1).length(), ' ') << "   " << std::string(m_idx, ' ') << ANSI_BLUE "^" << std::string(m_len - 1, '~') << ANSI_RESET
                  << std::endl;
    }

    void MessageWithSource::print_warning() {
        std::cout << ANSI_YELLOW "warning" ANSI_RESET " " << m_file.string() << ':' << m_line + 1 << ':' << m_col + 1 << ": " << m_msg
                  << '\n' << m_line + 1 << " | " << m_src.substr(0, m_idx) << ANSI_YELLOW << m_src.substr(m_idx, m_len) << ANSI_RESET << m_src.substr(m_idx + m_len)
                  << '\n' << std::string(std::to_string(m_line + 1).length(), ' ') << "   " << std::string(m_idx, ' ') << ANSI_YELLOW "^" << std::string(m_len - 1, '~') << ANSI_RESET
                  << std::endl;
    }

    void MessageWithSource::print_error() {
        std::cout << ANSI_RED "error" ANSI_RESET " " << m_file.string() << ':' << m_line + 1 << ':' << m_col + 1 << ": " ANSI_YELLOW << m_msg << ANSI_RESET
                  << '\n' << m_line + 1 << " | ";

        if (m_idx >= m_src.length()) {
            std::cout << m_src << ANSI_RED_BG << std::string(m_len, ' ') << ANSI_RESET;
        } else {
            std::cout << m_src.substr(0, m_idx) << ANSI_RED << m_src.substr(m_idx, m_len) << ANSI_RESET
                      << m_src.substr(m_idx + m_len);
        }


        std::cout << '\n' << std::string(std::to_string(m_line + 1).length(), ' ') << "   " << std::string(m_idx, ' ') << ANSI_RED "^" << std::string(m_len - 1, '~') << ANSI_RESET
                  << std::endl;
    }

    void MessageWithSource::print() {
        switch (m_level) {
            case Level::Note:
                print_notice();
                return;
            case Level::Warning:
                print_warning();
                return;
            case Level::Error:
                print_error();
                return;
        }
    }
}
