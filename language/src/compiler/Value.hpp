#pragma once

#include "types/Type.hpp"

namespace language {
    class CompilerValue {
    private:
        int m_val;
        bool m_stack; // m_pos refer to stack offset, of register
        const types::Type *m_type;

    public:
        explicit CompilerValue(const types::Type *type) : m_type(type), m_val(-1), m_stack(false) {};

        [[nodiscard]] int value() const { return m_val; }

        [[nodiscard]] bool is_stack() const { return m_stack; }

        void set(int value, bool is_stack) { m_stack = is_stack; m_val = value; }
    };
}
