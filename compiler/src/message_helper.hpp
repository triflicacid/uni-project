#pragma once

#include "messages/list.hpp"
#include "lexer/token.hpp"
#include "optional_ref.hpp"

namespace lang::symbol {
  class Symbol;
}

namespace lang::type {
  class Node;
}

namespace lang::value {
  class Value;
}

namespace lang::util {
  /**
   * @brief Builds an error message reporting that a named symbol could not be resolved.
   * @param source Location the error is attributed to.
   * @param name Name of the symbol that was not found.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_symbol_not_found(const message::MessageGenerator& source, const std::string& name);

  /**
   * @brief Builds an error message reporting that one type cannot be assigned or converted to another.
   * @param source Location the error is attributed to.
   * @param a Source type.
   * @param b Target type.
   * @param is_assignment True to phrase the message as an assignment failure, false as a conversion failure.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_type_mismatch(const message::MessageGenerator& source, const type::Node& a, const type::Node& b, bool is_assignment);

  /**
   * @brief Appends an error and supporting notes reporting mismatched branch types in an if-statement, plus notes pointing at each branch.
   * @param messages Message list to append to.
   * @param if_source Location of the if-statement itself.
   * @param then_source Location of the 'then' branch.
   * @param then_type Type produced by the 'then' branch.
   * @param else_source Location of the 'else' branch.
   * @param else_type Type produced by the 'else' branch, or empty if there is no explicit 'else' (implicit unit type).
   */
  void error_if_statement_mismatch(message::List& messages, const message::MessageGenerator& if_source, const message::MessageGenerator& then_source, const type::Node& then_type, const message::MessageGenerator& else_source, optional_ref<const type::Node> else_type);

  /**
   * @brief Builds an error message reporting that a type has no member with a given name.
   * @param source Location the error is attributed to.
   * @param type_a Type being accessed.
   * @param a Name/printed form of the value whose type is being accessed.
   * @param b Name of the member that does not exist.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_no_member(const message::MessageGenerator& source, const type::Node& type_a, const std::string& a, const std::string& b);

  /**
   * @brief Builds an error message reporting that an overloaded symbol reference is ambiguous despite having matching candidates.
   * @param source Location the error is attributed to.
   * @param name Name of the ambiguous symbol.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_ambiguous_reference(const message::MessageGenerator& source, const std::string& name);

  /**
   * @brief Appends one note message per candidate, pointing at each overload's definition site.
   * @param candidates Overload candidates to report.
   * @param messages Message list to append the notes to.
   */
  void note_candidates(const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates, message::List& messages);

  /**
   * @brief Builds an error message reporting that no overload of a symbol matches a required type hint.
   * @param source Location the error is attributed to.
   * @param name Name of the symbol being resolved.
   * @param type_hint Type hint that no overload matched.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_cannot_match_type_hint(const message::MessageGenerator& source, const std::string& name, const type::Node& type_hint);

  /**
   * @brief Builds an error message reporting invalid use of the discard symbol '_' as a name.
   * @param source Location the error is attributed to.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_underscore_bad_use(const message::MessageGenerator& source);

  /**
   * @brief Builds an error message reporting that an l-value or r-value was expected but the other kind was found.
   * @param source Location the error is attributed to.
   * @param type Type of the value that failed the check.
   * @param expected_lvalue True if an l-value was expected, false if an r-value was expected.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_expected_lrvalue(const message::MessageGenerator& source, const type::Node& type, bool expected_lvalue);

  /**
   * @brief Builds a "while evaluating ..." note message, used to give context to a preceding error.
   * @param source Location the note is attributed to.
   * @param addendum Optional text describing what was being evaluated; defaults to "this" when absent.
   * @return The constructed note message.
   */
  std::unique_ptr<message::Message> note_while_evaluating(const message::MessageGenerator& source, std::optional<std::string> addendum);

  /**
   * @brief Builds an error message reporting that a literal cannot be represented in a given type.
   * @param source Location the error is attributed to.
   * @param literal Textual form of the literal.
   * @param type Type the literal could not be stored in.
   * @return The constructed error message.
   */
  std::unique_ptr<message::Message> error_literal_bad_type(const message::MessageGenerator& source, const std::string& literal, const type::Node& type);
}
