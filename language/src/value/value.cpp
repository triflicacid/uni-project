#include "symbol.hpp"
#include "context.hpp"
#include "message_helper.hpp"

std::unique_ptr<lang::value::Symbol> lang::value::SymbolRef::resolve(lang::Context& ctx, const lexer::Token& token, bool generate_messages) const {
  const auto candidates = ctx.symbols.find(name_);

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    if (generate_messages) ctx.messages.add(util::error_unresolved_symbol(token, name_));
    return nullptr;
  }

  // if more than one candidate, we do not have sufficient info to choose
  if (candidates.size() > 1) {
    if (generate_messages) ctx.messages.add(util::error_insufficient_info_to_resolve_symbol(token, name_));
    return nullptr;
  }

  // fetch symbol
  symbol::Symbol& symbol = candidates.front().get();

  // create Value instance (symbol so lvalue)
  auto location = ctx.symbols.locate(symbol.id());
  return std::make_unique<value::Symbol>(symbol, value::Options{.lvalue=location, .computable=location.has_value()});
}
