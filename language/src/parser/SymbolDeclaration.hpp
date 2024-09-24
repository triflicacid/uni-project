#pragma once

#include <utility>

#include "types/Type.hpp"

namespace language::parser {
  class SymbolDeclaration {
    private:
    int m_pos;
    std::string m_name;
    int m_offset;
    bool m_is_arg; // Depends on how we access the variable.
    const types::Type *m_type;

    public:
    bool was_assigned;
    bool was_used_since_assignment;
    int last_assigned_pos;

    SymbolDeclaration(int pos, std::string name, const types::Type *type, bool is_arg)
        : m_type(type), m_pos(pos), m_name(std::move(name)), m_offset(-1), was_assigned(false),
          was_used_since_assignment(false), last_assigned_pos(-1), m_is_arg(is_arg) {};

    [[nodiscard]] std::string name() const { return m_name; }

    [[nodiscard]] const types::Type *type() const { return m_type; }

    [[nodiscard]] size_t size() const { return m_type->size(); }

    [[nodiscard]] int position() const { return m_pos; }

    [[nodiscard]] int offset() const { return m_offset; }

    void set_offset(int o) { m_offset = o; }

    [[nodiscard]] bool is_arg() const { return m_is_arg; }

    void debug_print(std::ostream &stream, const std::string &prefix) const;
  };
}
