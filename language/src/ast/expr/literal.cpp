#include "literal.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "ast/types/pointer.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "uint64.hpp"

const lang::memory::Literal& lang::ast::LiteralNode::get() const {
  assert(lit_.has_value());
  return *lit_;
}

std::ostream &lang::ast::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << token_start().image;
}

std::ostream &lang::ast::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_start().image << SHELL_RESET ": " SHELL_CYAN;

  if (lit_.has_value()) {
    lit_->get().type().print_code(os);
  } else {
    os << "?";
  }
  return os << SHELL_RESET;
}

const lang::ast::type::Node& lang::ast::LiteralNode::get_target_numeric_type() const {
  if (type_hint().has_value() && (type_hint()->get().get_int() || type_hint()->get().get_float())) {
    return type_hint()->get();
  } else if (token_start().type == lexer::TokenType::float_lit) {
    return ast::type::float32;
  } else {
    return ast::type::int32;
  }
}

bool lang::ast::LiteralNode::process(lang::Context& ctx) {
  if (!lit_.has_value()) {
    // determine type to cast to
    if (token_start().type == lexer::TokenType::null_kw) {
      const type::Node& target = type_hint().has_value() && (type_hint()->get().get_pointer() || type_hint()->get().get_func() || type_hint()->get().get_int())
          ? type_hint()->get()
          : type::PointerNode::get(type::uint8);
      lit_ = memory::Literal::get(target, 0x0);
    } else {
      const type::Node& target = get_target_numeric_type();
      const std::string& image = token_start().image;
      uint64_t value;
      bool is_error = false;

      try {
        if (target == type::uint8) {
          value = uint64::from(static_cast<uint8_t>(std::stoul(image)));
        } else if (target == type::int8) {
          value = uint64::from(static_cast<int8_t>(std::stoi(image)));
        } else if (target == type::uint16) {
          value = uint64::from(static_cast<uint16_t>(std::stoul(image)));
        } else if (target == type::int16) {
          value = uint64::from(static_cast<int16_t>(std::stoi(image)));
        } else if (target == type::uint32) {
          value = uint64::from(static_cast<uint32_t>(std::stoul(image)));
        } else if (target == type::int32) {
          value = uint64::from(static_cast<int32_t>(std::stoi(image)));
        } else if (target == type::uint64 || target.get_pointer()) {
          value = uint64::from(static_cast<uint64_t>(std::stoull(image)));
        } else if (target == type::int64) {
          value = uint64::from(static_cast<int64_t>(std::stoll(image)));
        } else if (target == type::float32) {
          value = uint64::from(std::stof(image));
        } else if (target == type::float64) {
          value = uint64::from(std::stod(image));
        } else {
          value = 0;
          is_error = true;
        }
      } catch (std::exception e) {
        is_error = true;
      }

      // check if there was an error
      if (is_error) {
        auto msg = generate_message(message::Error);
        msg->get() << "literal '" << image << "' is unable to be stored in type ";
        target.print_code(msg->get());
        ctx.messages.add(std::move(msg));
        return false;
      }

      // fetch literal matching the type and value
      lit_ = memory::Literal::get(target, value);
    }
  }

  value_ = value::rvalue(get().type());
  return true;
}

bool lang::ast::LiteralNode::resolve_rvalue(Context& ctx) {
  const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(get());
  value_->rvalue(std::make_unique<value::Literal>(get(), ref));
  return true;
}
