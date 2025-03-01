#include "array.hpp"
#include "pointer.hpp"
#include "int.hpp"
#include "context.hpp"
#include "value/future.hpp"

lang::ast::type::ArrayNode::ArrayNode(const lang::ast::type::Node& inner, size_t size)
  : WrapperNode("array", inner), size_(size) {}

size_t lang::ast::type::ArrayNode::size() const {
  return unwrap().size() * size_;
}

std::ostream& lang::ast::type::ArrayNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "[";
  unwrap().print_code(os) << "; " << size_ << "]";
  return os;
}

const lang::ast::type::ArrayNode& lang::ast::type::ArrayNode::get(const lang::ast::type::Node& inner, size_t size) {
  return WrapperNode::get<ArrayNode>("array", inner, [&] {
    return std::make_unique<ArrayNode>(inner, size);
  }, [&](const ArrayNode& other) {
    return other.size_ == size;
  });
}

const lang::ast::type::PointerNode& lang::ast::type::ArrayNode::decay_into_pointer() const {
  return PointerNode::get(unwrap());
}

static const std::string length_property = "length";

optional_ref<const lang::ast::type::Node>
lang::ast::type::ArrayNode::get_property_type(const std::string& property) const {
  if (property == length_property) {
    return uint64;
  }

  return Node::get_property_type(property);
}

std::unique_ptr<lang::value::Value> lang::ast::type::ArrayNode::get_property(Context& ctx, const std::string& property) const {
  if (property == length_property) {
    return value::literal(memory::Literal::get(uint64, size_));
  }

  return Node::get_property(ctx, property);
}
