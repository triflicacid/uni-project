#pragma once

#include <string>
#include <ostream>
#include <deque>
#include <memory>
#include "line.hpp"

namespace lang::assembly {
  class BytesDirective;
  class StringDirective;
  class DataDirective;
  class WordDirective;

  /**
   * @brief Represents a ".<directive> ..." assembly line, storing just the directive's name.
   *
   * Also hosts the static factory methods that are the intended way client code
   * constructs directives.
   */
  // represent a `.<directive> ...` entry
  class Directive : public Line {
    std::string name_;

  protected:
    std::ostream& _print(std::ostream &os) const override;

  public:
    /**
     * @brief Constructs a directive with a given name, without its leading dot.
     * @param name Directive name.
     */
    explicit Directive(std::string name) : name_(std::move(name)) {}

    /**
     * @brief Constructs an empty byte-segment directive (.byte).
     * @return The newly created directive.
     */
    // reserve a segment of bytes (uint8)
    static std::unique_ptr<BytesDirective> bytes();

    /**
     * @brief Constructs a byte-segment directive pre-populated with a string's characters.
     * @param str String whose characters populate the directive.
     * @return The newly created directive.
     */
    // reserve data for a string
    static std::unique_ptr<StringDirective> string(const std::string& str);

    /**
     * @brief Constructs an empty 32-bit-word data-segment directive (.data).
     * @return The newly created directive.
     */
    // reserve a segment of integers (uint32)
    static std::unique_ptr<DataDirective> data();

    /**
     * @brief Constructs an empty 64-bit-word data-segment directive (.word).
     * @return The newly created directive.
     */
    // reserve a segment of words (uint64)
    static std::unique_ptr<WordDirective> words();

    /**
     * @brief Constructs a .space directive reserving n bytes of uninitialized space.
     * @param n Number of bytes to reserve.
     * @return The newly created directive.
     */
    // reserve `n` bytes of empty space
    static std::unique_ptr<Directive> space(uint32_t n);

    /**
     * @brief Constructs an .offset directive repositioning the insertion cursor to absolute offset n.
     * @param n Absolute offset to reposition to.
     * @return The newly created directive.
     */
    // set insert point to `n`, use with extreme care
    static std::unique_ptr<Directive> offset(uint32_t n);
  };

  /**
   * @brief Internal template base for directives storing a deque of numeric values, rendered as hex-formatted data.
   * @tparam T Element type stored by the directive (uint8_t, uint32_t, or uint64_t).
   */
  template<typename T>
  class _DataDirective : public Directive {
  protected:
    std::deque<T> data_;

    std::ostream& _print(std::ostream &os) const override {
      Directive::print(os);
      for (const T& x : data_) os << " 0x" << std::hex << (int64_t) x;
      return os << std::dec;
    }

  public:
    /**
     * @brief Constructs an empty data directive with no initial elements.
     * @param name Directive name.
     */
    explicit _DataDirective(std::string name) : Directive(std::move(name)) {}

    /**
     * @brief Constructs a data directive pre-populated with a full set of elements.
     * @param name Directive name.
     * @param data Initial elements.
     */
    _DataDirective(std::string name, std::deque<T> data) : Directive(std::move(name)), data_(std::move(data)) {}

    /**
     * @brief Appends a single element to the directive's data.
     * @param x Element to append.
     * @return This directive, for chaining.
     */
    _DataDirective<T>& add(T x)
    { data_.push_back(std::move(x)); return *this; }

    /**
     * @brief Appends a whole batch of elements at once.
     * @param xs Elements to append.
     * @return This directive, for chaining.
     */
    _DataDirective<T>& add(const std::deque<T>& xs)
    { data_.insert(data_.end(), xs.begin(), xs.end()); return *this; }
  };

  /**
   * @brief Concrete _DataDirective<uint8_t> representing a .byte directive, with convenience overloads for populating from characters/strings.
   */
  struct BytesDirective : _DataDirective<uint8_t> {
    using _DataDirective::_DataDirective;

    /**
     * @brief Appends a single character as a raw byte, preserving its exact bit pattern.
     * @param x Character to append.
     * @return This directive, for chaining.
     */
    BytesDirective& add(char x);

    /**
     * @brief Appends every character of a string as consecutive raw bytes.
     * @param str String whose characters are appended.
     * @return This directive, for chaining.
     */
    BytesDirective& add(const std::string& str);
  };

  /**
   * @brief BytesDirective subclass that renders its byte buffer as quoted string segments interleaved with explicit hex bytes, followed by a trailing null terminator.
   */
  struct StringDirective : BytesDirective {
    using BytesDirective::BytesDirective;

    std::ostream& _print(std::ostream &os) const override;
  };

  /**
   * @brief Concrete _DataDirective<uint32_t> representing a .data directive.
   */
  struct DataDirective : _DataDirective<uint32_t> {
    using _DataDirective::_DataDirective;
  };

  /**
   * @brief Concrete _DataDirective<uint64_t> representing a .word directive.
   */
  struct WordDirective : _DataDirective<uint64_t> {
    using _DataDirective::_DataDirective;
  };

  /**
   * @brief Internal helper directive taking exactly one numeric parameter, used to implement both .space and .offset.
   */
  class _SingleDirective : public Directive {
    uint32_t n_;

  protected:
    std::ostream& _print(std::ostream &os) const override;

  public:
    /**
     * @brief Constructs a single-parameter directive.
     * @param name Directive name.
     * @param n Numeric parameter value.
     */
    _SingleDirective(std::string name, uint32_t n) : Directive(std::move(name)), n_(n) {}
  };
}
