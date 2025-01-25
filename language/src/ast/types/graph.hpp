#pragma once

#include "shared/graph.hpp"
#include "node.hpp"

namespace lang::ast::type {
  class TypeGraph {
    Graph<TypeId, std::reference_wrapper<Node>> graph_;

  public:
    // register a new type
    void insert(Node& type);

    // create a relationship a -> b, meaning `a is a subtype of b`
    void add_subtype(TypeId child, TypeId parent);

    // create relationships a -> b, a -> c, ...
    void add_subtypes(TypeId child, const std::vector<TypeId>& parents);

    // create relationships a -> z, b -> z, ...
    void add_subtypes(const std::vector<TypeId>& children, TypeId parent);

    // add a subtype chain a -> b -> c -> ...
    void add_subtype_chain(const std::vector<TypeId>& chain);

    // test if `child` is a subtype of `parent`
    bool is_subtype(TypeId child, TypeId parent) const;

    // initialise type dependency graph (type::graph)
    static void init();
  };

  extern TypeGraph graph;
}
