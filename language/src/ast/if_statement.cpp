#include "if_statement.hpp"
#include "types/node.hpp"
#include "context.hpp"
#include "control-flow/conditional_context.hpp"
#include "assembly/create.hpp"
#include "message_helper.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/bool.hpp"
#include "ast/types/unit.hpp"

// id used for `if` and `else` blocks
static int current_id = 0;

lang::ast::IfStatementNode::IfStatementNode(lang::lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> then_body,
                                            std::optional<std::unique_ptr<Node>> else_body)
    : Node(std::move(token)), guard_(std::move(guard)), then_(std::move(then_body)), else_(std::move(else_body)), id_(current_id++) {
  token_end(else_.has_value() ? else_.value()->token_end() : then_->token_end());
}

std::ostream& lang::ast::IfStatementNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "if ";
  guard_->print_code(os, indent_level) << " ";
  then_->print_code(os, indent_level + 1);

  if (else_.has_value()) {
    os << std::endl;
    indent(os, indent_level) << "else ";
    else_.value()->print_code(os, indent_level + 1);
  }

  return os;
}

std::ostream& lang::ast::IfStatementNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << std::endl;
  guard_->print_tree(os, indent_level + 1) << std::endl;
  then_->print_tree(os, indent_level + 1);

  if (else_.has_value()) {
    os << std::endl;
    else_.value()->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::IfStatementNode::always_returns() const {
  return then_->always_returns() && (!else_.has_value() || else_.value()->always_returns());
}

bool lang::ast::IfStatementNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return guard_->collate_registry(messages, registry)
    && then_->collate_registry(messages, registry)
    && (!else_.has_value() || else_.value()->collate_registry(messages, registry));
}

bool lang::ast::IfStatementNode::process(lang::Context& ctx) {
  // process guard, then, and else blocks
  if (!guard_->process(ctx)
    || !then_->process(ctx)
    || (else_.has_value() && !else_.value()->process(ctx))) return false;

  // resolve the guard
  if (!guard_->resolve(ctx)) return false;

  // check that types of branches match (unless one or both return)
  then_->type_hint(type_hint());
  if (!then_->resolve(ctx)) return false;
  if (else_.has_value()) {
    const type::Node& then_type = then_->value().type();

    else_.value()->type_hint(type_hint());
    if (!else_.value()->resolve(ctx)) return false;
    const type::Node& else_type = else_.value()->value().type();

    // if one or both always return, update accordingly
    const bool then_returns = then_->always_returns(), else_returns = else_.value()->always_returns();
    if (then_returns && else_returns) {
      value_ = value::unit_value();
    } else if (then_returns) {
      value_ = value::value(else_.value()->value().type());
    } else if (else_returns) {
      value_ = value::value(then_->value().type());
    } else if (then_type != else_type) {
      // TODO allow subtyping here?
      // both sides do not return, so types must be equal
      util::error_if_statement_mismatch(ctx.messages, token_start(), then_->token_end(), then_type, else_.value()->token_end(), else_type);
      return false;
    } else {
      // both sides return and are type-equal
      value_ = value::value(then_->value().type());
    }
  } else {
    // then `else` implicitly returns (), so check the then branch also returns ()
    auto& type = then_->value().type();
    if (type != type::unit) {
      util::error_if_statement_mismatch(ctx.messages, token_start(), then_->token_end(), type, then_->token_end(), std::nullopt);
      return false;
    }

    // all id good, set value
    value_ = value::value(type);
  }

  return true;
}

bool lang::ast::IfStatementNode::generate_code(lang::Context& ctx) const {
  // create branch destination blocks
  auto then_block = assembly::BasicBlock::labelled("then_" + std::to_string(id_));
  auto else_block = else_.has_value()
                    ? assembly::BasicBlock::labelled("else_" + std::to_string(id_))
                    : nullptr;
  auto after_block = assembly::BasicBlock::labelled("after_" + std::to_string(id_));

  // create the conditional context instance and attach to our guard
  control_flow::ConditionalContext cond_ctx{
      .if_true = *then_block,
      .if_false = else_block ? *else_block : *after_block
  };
  guard_->conditional_context(cond_ctx);

  // generate the guard
  if (!guard_->generate_code(ctx)) return false;

  // if branching has not been handled, do it ourselves
  if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, *guard_, guard_->value())) return false;

  // generate then branch
  ctx.program.insert(assembly::Position::Next, std::move(then_block));
  if (!then_->generate_code(ctx)) return false;

  // if we return, don't bother any more on this branch
  if (!then_->always_returns()) {
    // move result, if any, to $ret
    if (auto& then_value = then_->value(); then_value.is_rvalue() && then_value.type().size() > 0) {
      memory::Ref result = ctx.reg_alloc_manager.guarantee_datatype(then_value.rvalue().ref(), then_value.type());
      ctx.program.current().add(assembly::create_load(
          constants::registers::ret,
          ctx.reg_alloc_manager.resolve_ref(result, true)
      ));
    }

    // jump to end of the if statement
    ctx.program.current().add(assembly::create_branch(assembly::Arg::label(*after_block)));
  }

  // generate else branch?
  if (else_.has_value()) {
    ctx.program.insert(assembly::Position::Next, std::move(else_block));
    if (!else_.value()->generate_code(ctx)) return false;
    // no need for branch, we'll fall through (needed for proper basic block semantics though)
    // ctx.program.current().add(assembly::create_branch(assembly::Arg::label(*after_block)));

    // move result, if any, to $ret
    if (!else_.value()->always_returns()) {
      if (auto& else_value = else_.value()->value(); else_value.is_rvalue() && else_value.type().size() > 0) {
        memory::Ref result = ctx.reg_alloc_manager.guarantee_datatype(else_value.rvalue().ref(), else_value.type());
        ctx.program.current().add(assembly::create_load(
            constants::registers::ret,
            ctx.reg_alloc_manager.resolve_ref(result, true)
        ));
      }
    }
  }

  // update $ret, if necessary, to contain 'return' value from if statement
  // note it doesn't matter if a branch contained a return statement - populating the alloc manager's $ret happens on function calls
  auto value = value::rvalue(
      value_->type(),
      memory::Ref::reg(constants::registers::ret)
  );
  value_->rvalue(value->rvalue().copy());
  ctx.reg_alloc_manager.update_ret(memory::Object(std::move(value)));

  // insert the after block to finish this up
  ctx.program.insert(assembly::Position::Next, std::move(after_block));

  return true;
}
