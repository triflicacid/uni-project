#include "Program.hpp"

namespace language {
  void Program::debug_print(std::ostream &stream, const std::string &prefix) {
    stream << prefix << "<Program>" << std::endl;
    m_scope->debug_print(stream, prefix + "  ");
    stream << std::endl << prefix << "  ";

    if (m_funcs.empty()) {
      stream << "<Functions />";
    } else {
      stream << "<Functions>" << std::endl;

      for (auto &func: m_funcs) {
        func.second->debug_print(stream, prefix + "    ");
        stream << std::endl;
      }

      stream << "  </Functions>";
    }

    stream << std::endl << prefix << "</Program>";
  }

  CompiledProgram *Program::compile(message::List &messages) {
    auto *compiled = new CompiledProgram();
    m_compiled = true;
    compiled->source = m_source;

    for (auto &f_pair: m_funcs) {
      if (f_pair.second->is_complete()) {

      } else {
        compiled->declared_funcs.insert(f_pair);
      }
    }

    return compiled;
  }
}
