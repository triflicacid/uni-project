#include "constants.hpp"

using namespace constants;

std::map<std::string, registers::reg> registers::map = {
        {"pc",   registers::pc},
        {"rpc",  registers::rpc},
        {"sp",   registers::sp},
        {"fp",   registers::fp},
        {"flag", registers::flag},
        {"isr",  registers::isr},
        {"imr",  registers::imr},
        {"ipc",  registers::ipc},
        {"ret",  registers::ret},
        {"k1",   registers::k1},
        {"k2",   registers::k2},
};

std::string registers::to_string(reg r) {
    if (r >= count)
        return "?";

    for (const auto &pair : map) {
        if (pair.second == r) return pair.first;
    }

    return "r" + std::to_string(r - r1 + 1);
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

        return static_cast<reg>(off - 1 + r1);
    }

    return {};
}

std::optional<std::string> constants::registers::describe(reg r) {
  switch (r) {
    case pc:
      return "Program counter, points to the next instruction word in memory.";
    case rpc:
      return "Return program counter, set to the current $pc on call to a procedure, jump back here when producer is exited.";
    case sp:
      return "Stack pointer, points to the top of the stack (specifically, points to the byte above the top of the stack).";
    case fp:
      return "Frame pointer, points to the top of the latest stack frame.";
    case flag:
      return "Contains various state information about the processor, such as is_running and the result of comparisons.";
    case isr:
      return "Interrupt status register, a set bit indicates that this interrupt is pending.";
    case imr:
      return "Interrupt mask register, a bitmask used to 'cancel out' or ignore pending interrupts in $isr.";
    case ipc:
      return "Interrupt program counter, set to $pc on an interrupt, used to restore it on return from the interrupt handler.";
    case ret:
      return "Return register, stores the return value from the procedure/process, or stores error information on error.";
    case k1:
    case k2:
      return "An internal register used by pseudo-instructions and as scratch registers for the interrupt handler.";
    default:
      return "A general-purpose register, free for the programmers use.";
  }
}

std::unordered_map<std::string, cmp::flag> constants::cmp::map = {
        {"z",   cmp::z},
        {"nz",  cmp::nz},
        {"neq", cmp::ne},
        {"ne",  cmp::ne},
        {"eq",  cmp::eq},
        {"lte", cmp::le},
        {"lt",  cmp::lt},
        {"le",  cmp::le},
        {"gte", cmp::ge},
        {"gt",  cmp::gt},
        {"ge",  cmp::ge},
};

std::string cmp::to_string(flag v) {
    for (const auto &pair : map) {
        if (pair.second == v) return pair.first;
    }

    return "?";
}

std::optional<cmp::flag> cmp::from_string(const std::string &s) {
    int i = 0;
    return from_string(s, i);
}

std::optional<cmp::flag> cmp::from_string(const std::string &s, int &i) {
    for (const auto &pair : map) {
        if (s.substr(i, pair.first.length()) == pair.first) {
            i += (int) pair.first.length();
            return pair.second;
        }
    }

    return {};
}

std::unordered_map<std::string, inst::datatype::dt> inst::datatype::map = {
        {"hu", u32},
        {"u",  u64},
        {"hi", s32},
        {"i",  s64},
        {"f",  flt},
        {"d",  dbl}
};

std::string inst::datatype::to_string(dt v) {
    for (const auto &pair : map) {
        if (pair.second == v) return pair.first;
    }

    return "?";
}

std::optional<inst::datatype::dt> inst::datatype::from_string(const std::string &s) {
    int i = 0;
    return from_string(s, i);
}

std::optional<inst::datatype::dt> inst::datatype::from_string(const std::string &s, int &i) {
    for (const auto &pair : map) {
        if (s.substr(i, pair.first.length()) == pair.first) {
            i += (int) pair.first.length();
            return pair.second;
        }
    }

    return {};
}
