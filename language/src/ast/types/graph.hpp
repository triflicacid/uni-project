#pragma once

#include "shared/graph.hpp"
#include "node.hpp"

namespace lang::ast::type {
  class TypeGraph {
    std::deque<std::unique_ptr<Node>> store_; // used to store Types with no home :(
    Graph<TypeId, std::reference_wrapper<Node>> graph_;

  public:
    // store this type and add to graph
    void insert(std::unique_ptr<Node> type);

    // add a new type into the graph
    void insert(Node& type);

    // check if the given type exists
    bool exists(TypeId id) const;

    // lookup the type with the given ID, assume ID exists
    const Node& get(TypeId id) const;

    // lookup the type with the given ID, assume ID exists
    Node& get(TypeId id);

    // create a relationship a :> b, meaning `a is a subtype of b`
    void add_subtype(TypeId child, TypeId parent);

    // create relationships a :> b, a :> c, ...
    void add_subtypes(TypeId child, const std::vector<TypeId>& parents);

    // create relationships a :> z, b :> z, ...
    void add_subtypes(const std::vector<TypeId>& children, TypeId parent);

    // add a subtype chain a :> b :> c :> ...
    void add_subtype_chain(const std::vector<TypeId>& chain);

    // test if `child` is a subtype of `parent`
    bool is_subtype(TypeId child, TypeId parent) const;

    // initialise type dependency graph (type::graph)
    static void init();

    auto begin() { return graph_.begin(); }
    auto begin() const { return graph_.begin(); }
    auto end() { return graph_.end(); }
    auto end() const { return graph_.end(); }
  };

  extern TypeGraph graph;
}
