#pragma once

#include "node.hpp"
#include "graph.hpp"

namespace lang::ast::type {
  class ConstWrapperNode;

  // a wrapper wraps a type
  class WrapperNode : public Node {
    std::string name_;
    const Node& inner_;

  public:
    WrapperNode(std::string name, const Node& inner) : name_(std::move(name)), inner_(inner) {}

    std::string name() const override { return name_; }

    const WrapperNode* get_wrapper() const override { return this; }

    virtual const ConstWrapperNode* get_const() const { return nullptr; }

    const Node& unwrap() const { return inner_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::string to_label() const override;

    // get reference to a wrapper type, or create it
    template <class T> requires std::is_base_of_v<WrapperNode, T>
    static const T& create(const std::string& wrapper_name, const Node& inner_type, const std::function<std::unique_ptr<T>()>& create_type) {
      for (auto& [id, type] : graph) {
        if (auto wrapper_type = type.get().get_wrapper()) {
          if (wrapper_type->name() == wrapper_name && wrapper_type->unwrap().id() == inner_type.id()) {
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
