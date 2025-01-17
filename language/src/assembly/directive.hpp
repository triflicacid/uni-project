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

  // represent a `.<directive> ...` entry
  class Directive : public Line {
    std::string name_;

  public:
    explicit Directive(std::string name) : name_(std::move(name)) {}

    std::ostream& print(std::ostream &os) const override;

    // reserve a segment of bytes (uint8)
    static std::unique_ptr<BytesDirective> bytes();

    // reserve data for a string
    static std::unique_ptr<StringDirective> string(const std::string& str);

    // reserve a segment of integers (uint32)
    static std::unique_ptr<DataDirective> data();

    // reserve a segment of words (uint64)
    static std::unique_ptr<WordDirective> words();

    // reserve `n` bytes of empty space
    static std::unique_ptr<Directive> space(uint32_t n);

    // set insert point to `n`, use with extreme care
    static std::unique_ptr<Directive> offset(uint32_t n);
  };

  template<typename T>
  class _DataDirective : public Directive {
  protected:
    std::deque<T> data_;

  public:
    explicit _DataDirective(std::string name) : Directive(std::move(name)) {}

    _DataDirective(std::string name, std::deque<T> data) : Directive(std::move(name)), data_(std::move(data)) {}

    _DataDirective<T>& add(T x)
    { data_.push_back(std::move(x)); return *this; }

    _DataDirective<T>& add(const std::deque<T>& xs)
    { data_.insert(data_.end(), xs.begin(), xs.end()); return *this; }

    std::ostream& print(std::ostream &os) const override {
      Directive::print(os);
      for (const T& x : data_) os << " 0x" << std::hex << (int64_t) x;
      return os << std::dec;
    }
  };

  struct BytesDirective : _DataDirective<uint8_t> {
    using _DataDirective::_DataDirective;

    BytesDirective& add(char x);

    BytesDirective& add(const std::string& str);
  };

  struct StringDirective : BytesDirective {
    using BytesDirective::BytesDirective;

    std::ostream& print(std::ostream &os) const override;
  };

  struct DataDirective : _DataDirective<uint32_t> {
    using _DataDirective::_DataDirective;
  };

  struct WordDirective : _DataDirective<uint64_t> {
    using _DataDirective::_DataDirective;
  };

  class _SingleDirective : public Directive {
    uint32_t n_;

  public:
    _SingleDirective(std::string name, uint32_t n) : Directive(std::move(name)), n_(n) {}

    std::ostream& print(std::ostream &os) const override;
  };
}
