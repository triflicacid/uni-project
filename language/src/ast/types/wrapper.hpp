#pragma once

#include "node.hpp"
#include "graph.hpp"

namespace lang::ast::type {
  // a wrapper wraps a type
  class WrapperNode : public Node {
    std::string name_;
    const Node& inner_;

  public:
    WrapperNode(std::string name, const Node& inner) : name_(std::move(name)), inner_(inner) {}

    std::string node_name() const override { return name_; }

    const WrapperNode* get_wrapper() const override { return this; }

    const Node& unwrap() const { return inner_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::string to_label() const override;

    // get reference to a wrapper type, or create it
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
