#include "array.hpp"
#include "pointer.hpp"
#include "int.hpp"
#include "context.hpp"
#include "value/future.hpp"

lang::type::ArrayNode::ArrayNode(const Node& inner, size_t size)
  : WrapperNode("array", inner), size_(size) {}

size_t lang::type::ArrayNode::size() const {
  return unwrap().size() * size_;
}

std::ostream& lang::type::ArrayNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "[";
  unwrap().print_code(os) << "; " << size_ << "]";
  return os;
}

const lang::type::ArrayNode& lang::type::ArrayNode::get(const Node& inner, size_t size) {
  return WrapperNode::get<ArrayNode>("array", inner, [&] {
    return std::make_unique<ArrayNode>(inner, size);
  }, [&](const ArrayNode& other) {
    return other.size_ == size;
  });
}

const lang::type::PointerNode& lang::type::ArrayNode::decay_into_pointer() const {
  return PointerNode::get(unwrap());
}

static const std::string length_property = "length";

optional_ref<const lang::type::Node>
lang::type::ArrayNode::get_property_type(const std::string& property) const {
  if (property == length_property) {
    return uint64;
  }

  return Node::get_property_type(property);
}

std::unique_ptr<lang::value::Value> lang::type::ArrayNode::get_property(Context& ctx, const std::string& property) const {
  if (property == length_property) {
    return value::literal(memory::Literal::get(uint64, size_));
  }

  return Node::get_property(ctx, property);
}
