#pragma once

#include <string>
#include <utility>
#include <vector>
#include "location.hpp"

namespace message {
    enum Level {
        Note,
        Warning,
        Error
    };

    class Message {
    protected:
        Location m_loc;
        Level m_level;
        std::stringstream m_msg;

        /** Print varying type line e.g., 'ERROR!' */
        void print_type_suffix(std::ostream &os);

    public:
        Message(Level level, Location loc) : m_level(level), m_loc(std::move(loc)) {}

        std::stringstream &get() { return m_msg; }

        /** Get message code, or -1. */
        virtual int get_code() { return -1; }

        Level get_level() { return m_level; }

        virtual void print(std::ostream &os);
    };

    /** Get level from int, where lowest is 0 */
    Level level_from_int(int level);
}
