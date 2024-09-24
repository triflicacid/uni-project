#include "SymbolRef.hpp"
#include "util/util.h"
#include "FunctionCall.hpp"
#include "Number.hpp"
#include "ReturnStatement.hpp"

namespace language::statement {
    void Number::compile_push(std::ostream &stream, CompileData *cdata, message::List &messages) const {
        if (m_type->is_float()) {
            // TODO
        } else {
            stream << "psh" << m_type->bits() << " " << m_token->image() << std::endl;
            cdata->add_stack_offset((int) m_type->size());
        }
    }

    void FunctionCall::compile(std::ostream &stream, CompileData *cdata, message::List &messages) const {
        int before = cdata->stack_offset();

        // Evaluate arguments
        for (int i = 0; i < m_args.size(); i++) {
            auto *arg = m_args[i];
            auto *arg_type = m_func->function_type()->arg(i);

            if (arg->type() == Type::NUMBER) {
                ((Number *) arg)->compile_push(stream, cdata, messages);
            } else {
                arg->compile(stream, cdata, messages);


            }
        }

        stream << "cal " << m_func->to_label() << std::endl;

        cdata->set_stack_offset(before);
    }

    void ReturnStatement::compile(std::ostream &stream, CompileData *cdata, message::List &messages) const {
        Statement::compile(stream, cdata, messages);
    }

    void SymbolRef::compile(std::ostream &stream, int reg) const {
        if (m_decl->is_arg()) {
            int offset = 2 * 8 + m_decl->offset();
            stream << "mov [fp], " << offset << ", r" << reg << std::endl;
        } else {
            stream << "mov [sp], " << m_decl->offset() << ", r" << reg << std::endl;
        }
    }

    void SymbolRef::compile(std::ostream &stream, CompileData *cdata, message::List &messages) const {
        compile(stream, cdata->get_register());
    }
}
