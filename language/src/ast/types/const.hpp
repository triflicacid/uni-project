#include "wrapper.hpp"

namespace lang::ast::type {
  class ConstWrapperNode : public WrapperNode {
  public:
    explicit ConstWrapperNode(const Node& inner) : WrapperNode("const", inner) {}

    size_t size() const override { return unwrap().size(); }

    constants::inst::datatype::dt get_asm_datatype() const override { return unwrap().get_asm_datatype(); }

    const ConstWrapperNode* get_const() const override { return this; }
  };

  const ConstWrapperNode& constant(const Node& inner) {
    return WrapperNode::create<ConstWrapperNode>("const", inner, [&] { return std::make_unique<ConstWrapperNode>(inner); });
  }
}
