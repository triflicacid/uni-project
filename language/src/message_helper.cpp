#include "message_helper.hpp"
#include "symbol/table.hpp"
#include "config.hpp"

void lang::util::check_local_scope(const lang::symbol::SymbolTable& symbols, message::List& messages) {
  if (conf::lint) {
    for (symbol::SymbolId id: symbols.peek()) {
      const symbol::Symbol& symbol = symbols.get(id);

      if (symbol.ref_count() == 0) {
        auto msg = symbol.token().generate_message(conf::lint_level);
        msg->get() << symbol::category_to_string(symbol.category()) << " is unused, consider removing?";
        messages.add(std::move(msg));
      }
    }
  }
}
