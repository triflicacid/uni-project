#include "CompiledProgram.hpp"

namespace language {
    void CompiledProgram::debug_print(std::ostream &stream, const std::string &prefix) {
        stream << prefix << "<CompiledProgram>" << std::endl;
        stream << prefix << "  <Source>" << source->path().string() << "</Source>" << std::endl;

        if (!declared_funcs.empty()) {
            stream << prefix << "  <DeclaredFunctions>" << std::endl;

            for (auto& pair : declared_funcs) {
                stream << prefix << "    <Function id=\"" << pair.first << "\" name=\"" << pair.second->name() << "\">" << pair.second->to_label() << "</Function>" << std::endl;
            }

            stream << prefix << "  </DeclaredFunctions>" << std::endl;
        }

        if (!defined_funcs.empty()) {
            stream << prefix << "  <Functions>" << std::endl;

            for (auto& pair : defined_funcs) {
                stream << prefix << "    <Function id=\"" << pair.first << "\" name=\"" << pair.second.first->name() << "\">" << std::endl;
                stream << prefix << "      <Label>" << pair.second.first->to_label() << "</Label>" << std::endl;
                stream << prefix << "      <Body>" << pair.second.second.rdbuf() << "</Body>" << std::endl;
                stream << prefix << "    </Function>" << std::endl;
            }

            stream << prefix << "  </Functions>" << std::endl;
        }

        stream << prefix << "</CompiledProgram>";
    }
}
