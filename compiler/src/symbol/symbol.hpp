 #pragma once

#include <string>
#include "types/node.hpp"
#include "lexer/token.hpp"

 namespace lang::symbol {
  /**
   * @brief Globally-unique identifier assigned to each registered Symbol.
   */
  using SymbolId = uint32_t;

  /**
   * @brief What kind of name-bound entity a Symbol represents.
   */
  enum class Category {
    StackBased, ///< Ordinary symbol, stack-based; offset determined by StackManager.
    Global, ///< Global symbol.
    Argument, ///< Function argument; refers to its index.
    Function, ///< Globally-placed function (block-bound).
    Namespace, ///< Namespace, used purely for qualifying nested names.
  };

  /**
   * @brief Renders a Category enumerator as a human-readable word, for diagnostics.
   * @param category Category to render.
   * @return Word describing the category (e.g. "function", "namespace").
   */
  std::string category_to_string(Category category);

  /**
   * @brief Represents any name-bound entity: a variable, argument, function, or namespace.
   *
   * Tracks its origin token, optional parent symbol (for namespace-qualified names), a
   * unique id, its static type, and constant/assignment flags. define() is a lazy hook
   * that ensures the symbol has a concrete storage location or code-generated body.
   */
  class Symbol {
    lexer::Token token_; ///< Origin token (top-level name).
    std::optional<std::reference_wrapper<const Symbol>> parent_; ///< Parent symbol (i.e. enclosing namespace), if any.
    SymbolId id_ = -1; ///< Unique id.
    Category category_; ///< Kind of symbol this is.
    const type::Node& type_; ///< Static type of the symbol.
    bool constant_ = false; ///< Whether the symbol was declared constant.
    bool assigned_ = false; ///< Whether the symbol has been assigned to.

  public:
    Symbol(const Symbol&) = delete;

    /**
     * @brief Constructs a symbol with no explicit category.
     * @param name Origin token providing the symbol's name.
     * @param type_ Static type of the symbol.
     */
    Symbol(lexer::Token name, const type::Node& type_);

    /**
     * @brief Constructs a symbol with an explicit category.
     * @param name Origin token providing the symbol's name.
     * @param category Category describing what kind of symbol this is.
     * @param type_ Static type of the symbol.
     */
    Symbol(lexer::Token name, Category category, const type::Node& type_);

    virtual ~Symbol() = default;

    /**
     * @brief Returns the token the symbol's name was declared with.
     * @return The origin token.
     */
    const lexer::Token& token() const { return token_; }

    /**
     * @brief Returns the symbol's category.
     * @return The category.
     */
    const Category& category() const { return category_; }

    /**
     * @brief Returns the symbol's unqualified name.
     * @return The name as it appeared in source, without namespace qualification.
     */
    const std::string& name() const { return token_.image; }

    /**
     * @brief Marks the symbol as immutable.
     */
    void make_constant() { constant_ = true; }

    /**
     * @brief Reports whether the symbol was declared constant.
     * @return True if constant.
     */
    bool is_constant() const { return constant_; }

    /**
     * @brief Builds the fully-qualified, dot-separated name of the symbol by walking up its parent chain.
     * @return The qualified name.
     */
    // generate fully-qualified name (by tracking parents)
    std::string full_name() const;

    /**
     * @brief Returns the symbol's optional parent (enclosing namespace).
     * @return The parent symbol, or empty if there is none.
     */
    const std::optional<std::reference_wrapper<const Symbol>>& parent() const { return parent_; }

    /**
     * @brief Attaches a parent symbol, establishing namespace nesting.
     * @param parent Symbol to set as the parent.
     */
    void set_parent(const Symbol& parent) { parent_ = parent; }

    /**
     * @brief Returns the symbol's unique id.
     * @return The id.
     */
    uint32_t id() const { return id_; }

    /**
     * @brief Returns the symbol's static type.
     * @return The type.
     */
    const type::Node& type() const { return type_; }

    /**
     * @brief Ensures the symbol has a concrete storage location or code-generated body, allocating it on first use.
     * @param ctx Compiler context providing the symbol table used to check and perform allocation.
     * @return True on success.
     */
    // ensure that this symbol is defined
    // return success
    virtual bool define(Context& ctx) const;
  };

  /**
   * @brief Creates a new namespace symbol, marked constant.
   * @param name Origin token providing the namespace's name.
   * @return The newly created namespace symbol.
   */
  // create a new namespace
  std::unique_ptr<Symbol> create_namespace(const lexer::Token& name);
}
