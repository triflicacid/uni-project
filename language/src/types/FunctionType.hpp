#pragma once

#include "Type.hpp"
#include "util.h"
#include <ostream>

namespace language::types {
  class FunctionType : public Type {
    private:
    int m_pos;
    std::vector<const types::Type *> m_args; // Vector of argument type tokens
    const types::Type *m_ret;
    int m_id;

    public:
    FunctionType(int token_pos, const std::vector<const types::Type *> &args, const types::Type *ret)
        : Type(), m_pos(token_pos), m_args(args), m_ret(ret), m_id(-1) {};

    [[nodiscard]] size_t size() const override { return 8; }

    /** Check if arguments match with another type. */
    [[nodiscard]] bool matches_args(const FunctionType *other) const;

    /** Check if arguments match. */
    [[nodiscard]] bool matches_args(const std::vector<const types::Type *> &arguments) const;

    /** Check if two signatures are equal */
    [[nodiscard]] bool equal(const FunctionType &other) const;

    /** Get function ID. */
    [[nodiscard]] int id() const { return m_id; }

    /** Is function bound in the program? */
    [[nodiscard]] bool is_stored() const { return m_id > -1; }

    /** Set ID. */
    void set_id(int id) { m_id = id; }

    /** Get argument count. */
    [[nodiscard]] size_t argc() const { return m_args.size(); }

    [[nodiscard]] const types::Type *arg(int i) const { return m_args[i]; }

    /** Calculate arg block size. */
    [[nodiscard]] size_t arg_block_size() const;

    /** Calculate offset of each argument from fp. */
    [[nodiscard]] std::vector<int> arg_offsets() const;

    [[nodiscard]] int position() const { return m_pos; }

    [[nodiscard]] Category category() const override { return Category::Function; }

    [[nodiscard]] const types::Type *return_type() const { return m_ret; }

    /** String representation as in definition. */
    [[nodiscard]] std::string repr() const override;

    void debug_print(std::ostream &stream, const std::string &prefix) const override;

    /** Check if this function is a valid entry point. */
    static bool valid_entry_point(const FunctionType *type);
  };
}
