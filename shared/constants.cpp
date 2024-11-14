#include "constants.hpp"

using namespace constants;

std::map<std::string, registers::reg> registers::map = {
        {"ip",   registers::ip},
        {"rip",  registers::rip},
        {"sp",   registers::sp},
        {"fp",   registers::fp},
        {"flag", registers::flag},
        {"isr",  registers::isr},
        {"imr",  registers::imr},
        {"iip",  registers::iip},
        {"ret",  registers::ret},
        {"k1",   registers::k1},
        {"k2",   registers::k2},
};

std::string registers::to_string(registers::reg r) {
    if (r >= registers::count)
        return "?";

    for (const auto &pair : map) {
        if (pair.second == r) return pair.first;
    }

    return "r" + std::to_string(r - registers::r1 + 1);
}

std::optional<registers::reg> registers::from_string(const std::string &s) {
    int i = 0;
    return from_string(s, i);
}

std::optional<registers::reg> registers::from_string(const std::string &s, int &i) {
    for (const auto &pair : map) {
        if (s.substr(i, pair.first.length()) == pair.first) {
            i += (int) pair.first.size();
            return pair.second;
        }
    }

    if (s[i] == 'r') {
        i++;
        int off = 0;

        while (i < s.size() && std::isdigit(s[i]))
            off = 10 * off + (s[i++] - '0');

        return static_cast<reg>(off - 1 + registers::r1);
    }

    return {};
}
