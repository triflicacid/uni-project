#include "graph.hpp"
#include "int.hpp"
#include "float.hpp"

void lang::ast::type::TypeGraph::insert(lang::ast::type::Node& type) {
  graph_.insert(type.id(), type);
}

void lang::ast::type::TypeGraph::add_subtype(TypeId child, TypeId parent) {
  graph_.insert(parent, child);
}

void lang::ast::type::TypeGraph::add_subtypes(TypeId child, const std::vector<TypeId>& parents) {
  for (TypeId parent : parents) {
    add_subtype(child, parent);
  }
}

void lang::ast::type::TypeGraph::add_subtypes(const std::vector<TypeId>& children, TypeId parent) {
  for (TypeId child : children) {
    add_subtype(child, parent);
  }
}

void lang::ast::type::TypeGraph::add_subtype_chain(const std::vector<TypeId>& chain) {
  for (int i = chain.size() - 1; i > 0; i--) {
    add_subtype(chain[i-1], chain[i]);
  }
}

bool lang::ast::type::TypeGraph::is_subtype(lang::ast::type::TypeId child, lang::ast::type::TypeId parent) const {
  return graph_.are_connected(parent, child);
}

lang::ast::type::TypeGraph lang::ast::type::graph;

void lang::ast::type::TypeGraph::init() {
  // integer types
  graph.insert(uint8);
  graph.insert(int8);
  graph.insert(uint16);
  graph.insert(int16);
  graph.insert(uint32);
  graph.insert(int32);
  graph.insert(uint64);
  graph.insert(int64);

  // int8 < int16 < int32 < int64
  graph.add_subtype_chain({int8.id(), int16.id(), int32.id(), int64.id()});

  // uint8 < uint16 < uint32 < uint64
  graph.add_subtype_chain({uint8.id(), uint16.id(), uint32.id(), uint64.id()});

  // uint<n> < int<n+k>
  graph.add_subtypes(uint8.id(), {int16.id(), int32.id(), int64.id()});
  graph.add_subtypes(uint16.id(), {int32.id(), int64.id()});
  graph.add_subtype(uint32.id(), int64.id());

  // floating point types
  graph.insert(float32);
  graph.insert(float64);

  // float32 < float64
  graph.add_subtype(float32.id(), float64.id());

  // [u]int<n> < float64
  graph.add_subtypes({uint8.id(), int8.id(), uint16.id(), int16.id(), uint32.id(), int32.id(), uint64.id(), int64.id()}, float64.id());

  // [u]int<n> < float32 where n < 64
  graph.add_subtypes({uint8.id(), int8.id(), uint16.id(), int16.id(), uint32.id(), int32.id()}, float32.id());
}
