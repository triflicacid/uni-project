#include <cstdint>
#include "Type.hpp"
#include "UnitType.hpp"
#include "primitive.hpp"

namespace language::types {
  const UnitType UnitType::instance;
  const Byte Byte::instance;
  const Int Int::instance;
  const UInt UInt::instance;
  const Word Word::instance;
  const UWord UWord::instance;
  const Float Float::instance;
  const Double Double::instance;

  bool can_implicitly_cast_to(const Type *fst, const Type *snd) {
    // are types the same?
    if (fst == snd) return true;

    // both integers and sizes fit?
    if (fst->category() == Category::Integer && snd->category() == Category::Integer)  {
      return fst->size() <= snd->size();
    }

    return false;
  }

  const Type *get_suitable_int_type(uint64_t value) {
    if (value <= UCHAR_MAX) return &Byte::instance;
    if (value <= INT32_MAX) return &Int::instance;
    if (value <= INT64_MAX) return &Word::instance;

    return &UWord::instance;
  }
}
