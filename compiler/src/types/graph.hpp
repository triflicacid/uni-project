#pragma once

#include "shared/graph.hpp"
#include "node.hpp"

namespace lang::type {
  /**
   * @brief Owns and tracks all interned type instances plus their subtype relationships.
   *
   * Holds a store of heap-allocated types with no other owner, and a generic
   * Graph<TypeId, Node&> recording subtype edges (`add_subtype`/`is_subtype`).
   * The single global instance is `type::graph`; `init()` bootstraps the
   * built-in numeric/boolean subtype hierarchy.
   */
  class TypeGraph {
    std::deque<std::unique_ptr<Node>> store_; ///< Owns heap-allocated types that have no other owner.
    Graph<TypeId, std::reference_wrapper<Node>> graph_; ///< Subtype-relationship graph over every inserted type.

  public:
    /**
     * @brief Take ownership of a heap-allocated type and add it to the graph.
     * @param type Type to store and insert.
     */
    void insert(std::unique_ptr<Node> type);

    /**
     * @brief Add a type with an external owner (e.g. a global instance) into the graph.
     * @param type Type to insert.
     */
    void insert(Node& type);

    /**
     * @brief Test whether a type with the given id has been inserted.
     * @param id Type id to check.
     * @return True if it exists.
     */
    bool exists(TypeId id) const;

    /**
     * @brief Look up a type by id. Assumes the id exists.
     * @param id Type id to look up.
     * @return The corresponding type.
     */
    const Node& get(TypeId id) const;

    /**
     * @brief Look up a type by id. Assumes the id exists.
     * @param id Type id to look up.
     * @return The corresponding type.
     */
    Node& get(TypeId id);

    /**
     * @brief Record that `child` is a subtype of `parent`.
     * @param child Subtype's id.
     * @param parent Supertype's id.
     */
    void add_subtype(TypeId child, TypeId parent);

    /**
     * @brief Record that `child` is a subtype of each of `parents`.
     * @param child Subtype's id.
     * @param parents Supertypes' ids.
     */
    void add_subtypes(TypeId child, const std::vector<TypeId>& parents);

    /**
     * @brief Record that each of `children` is a subtype of `parent`.
     * @param children Subtypes' ids.
     * @param parent Supertype's id.
     */
    void add_subtypes(const std::vector<TypeId>& children, TypeId parent);

    /**
     * @brief Record a chain of subtype relationships a :> b :> c :> ...
     * @param chain Ids ordered from most-derived to least-derived.
     */
    void add_subtype_chain(const std::vector<TypeId>& chain);

    /**
     * @brief Test whether `child` is a subtype of (or identical to) `parent`.
     * @param child Candidate subtype's id.
     * @param parent Candidate supertype's id.
     * @return True if the subtype relationship holds.
     */
    bool is_subtype(TypeId child, TypeId parent) const;

    /** @brief Initialize the global type dependency graph (`type::graph`) with the built-in numeric/boolean types and their subtype relationships. */
    static void init();

    /** @brief Return an iterator to the first (id, type) pair. @return Iterator to the first element. */
    auto begin() { return graph_.begin(); }

    /** @brief Return a const iterator to the first (id, type) pair. @return Const iterator to the first element. */
    auto begin() const { return graph_.begin(); }

    /** @brief Return an iterator past the last (id, type) pair. @return Iterator to one-past-last element. */
    auto end() { return graph_.end(); }

    /** @brief Return a const iterator past the last (id, type) pair. @return Const iterator to one-past-last element. */
    auto end() const { return graph_.end(); }
  };

  extern TypeGraph graph; ///< The single global type-dependency graph, initialised via @ref TypeGraph::init.
}
