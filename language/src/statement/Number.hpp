#pragma once

#include "Statement.hpp"
#include "types/primitive.hpp"

namespace language::statement {
  class Number : public Expression {
    private:
    const types::Type *m_type;
    uint64_t m_data;

    public:
    Number(int pos, const lexer::Token *token, const types::Type *numeric_type, uint64_t data)
        : Expression(pos, token), m_type(numeric_type), m_data(data) {};

    /** return if float or double */
    [[nodiscard]] bool is_float() const { return m_type->category() == types::Category::Float; }

    [[nodiscard]] Type type() const override { return is_float() ? Type::FLOAT : Type::INTEGER; }

    [[nodiscard]] size_t size() const { return m_type->size(); }

    [[nodiscard]] const types::Type *get_type_of() const override { return m_type; }

    void debug_print(std::ostream &stream, const std::string &prefix) const override {
      stream << prefix << "<Number>" << std::endl;
      m_type->debug_print(stream, prefix + "  ");
      stream << std::endl << prefix << "  <Value>";

      if (is_float()) {
        if (m_type->size() == 4) {
          stream << *(float *) &m_data;
        } else {
          stream << *(double *) &m_data;
        }
      } else {
        stream << m_data;
      }

      stream << "</Value>" << std::endl;
      stream << prefix << "</Number>";
    }

    //void compile_push(std::ostream &stream, CompileData *cdata, message::List &messages) const override;
  };
}
