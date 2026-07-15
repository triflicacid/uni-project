#pragma once

#include "node.hpp"
#include "graph.hpp"

namespace lang::type {
  /** @brief Base class for types that wrap another type (arrays, pointers), storing a name and a reference to the wrapped inner type. */
  class WrapperNode : public Node {
    std::string name_; ///< Wrapper kind name, e.g. "pointer" or "array".
    const Node& inner_; ///< Wrapped inner type.

  public:
    /**
     * @brief Construct a wrapper type.
     * @param name Wrapper kind name, e.g. "pointer" or "array".
     * @param inner Wrapped inner type.
     */
    WrapperNode(std::string name, const Node& inner) : name_(std::move(name)), inner_(inner) {}

    /** @brief Return the wrapper kind name. */
    std::string node_name() const override { return name_; }

    /** @brief Return this node, since it is already a WrapperNode. */
    const WrapperNode* get_wrapper() const override { return this; }

    /** @brief Return the wrapped inner type. */
    const Node& unwrap() const { return inner_; }

    /**
     * @brief Print this type as `name<inner>` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Return the label representation, concatenating the wrapper's name and the inner type's label. */
    std::string to_label() const override;

    /**
     * @brief Find an existing wrapper type matching the given kind, inner type, and equality predicate in the global TypeGraph, or create and intern a new one.
     * @tparam T Concrete wrapper subtype being looked up/created.
     * @param wrapper_name Wrapper kind name to match.
     * @param inner_type Wrapped inner type to match.
     * @param create_type Factory invoked to build a new instance if no match is found.
     * @param is_equal Additional equality predicate distinguishing wrappers with the same name/inner type (e.g. array length).
     * @return Reference to the matching or newly created wrapper type.
     */
    template <class T> requires std::is_base_of_v<WrapperNode, T>
    static const T& get(const std::string& wrapper_name, const Node& inner_type, const std::function<std::unique_ptr<T>()>& create_type, std::optional<std::function<bool(const T&)>> is_equal = std::nullopt) {
      for (auto& [id, type] : graph) {
        if (auto wrapper_type = type.get().get_wrapper()) {
          if (wrapper_type->node_name() == wrapper_name && wrapper_type->unwrap() == inner_type && (!is_equal || is_equal.value()(*static_cast<const T*>(wrapper_type)))) {
            return *static_cast<const T*>(wrapper_type);
          }
        }
      }

      // otherwise, create type
      std::unique_ptr<Node> wrapper_type = create_type();
      const TypeId id = wrapper_type->id();
      graph.insert(std::move(wrapper_type));
      return static_cast<const T&>(graph.get(id));
    }
  };
}
