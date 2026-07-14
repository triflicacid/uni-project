#pragma once

#include <string>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>

/**
 * @brief Trim characters from the left (start) of a string, in place.
 * @param s String to trim.
 * @param t Set of characters to trim.
 * @return `s`, for chaining.
 */
std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v");

/**
 * @brief Trim characters from the right (end) of a string, in place.
 * @param s String to trim.
 * @param t Set of characters to trim.
 * @return `s`, for chaining.
 */
std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v");

/**
 * @brief Trim characters from both ends of a string, in place.
 * @param s String to trim.
 * @param t Set of characters to trim.
 * @return `s`, for chaining.
 */
std::string &trim(std::string &s, const char *t = " \t\n\r\f\v");

/**
 * @brief Convert a string to lowercase, in place.
 * @param str String to convert.
 */
inline void to_lowercase(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

/**
 * @brief Advance an index past whitespace characters.
 * @param s String being scanned.
 * @param i Index to advance; left just past the last whitespace character.
 */
void skip_whitespace(const std::string &s, int &i);

/**
 * @brief Advance an index past non-whitespace characters.
 * @param s String being scanned.
 * @param i Index to advance; left just past the last non-whitespace character.
 */
void skip_non_whitespace(const std::string &s, int &i);

/**
 * @brief Advance an index past characters that are neither whitespace nor a comma.
 * @param s String being scanned.
 * @param i Index to advance; left at the next whitespace character or comma.
 */
void skip_to_break(const std::string &s, int &i);

/**
 * @brief Advance an index past alphabetic characters.
 * @param s String being scanned.
 * @param i Index to advance; left just past the last alphabetic character.
 */
void skip_alpha(const std::string &s, int &i);

/**
 * @brief Advance an index past alphanumeric characters.
 * @param s String being scanned.
 * @param i Index to advance; left just past the last alphanumeric character.
 */
void skip_alphanum(const std::string &s, int &i);

/**
 * @brief Check whether a string starts with another string.
 * @param a String to check.
 * @param b Prefix to look for.
 * @return True if `a` starts with `b`.
 */
bool starts_with(const std::string &a, std::string b);

/**
 * @brief Check whether a string, from a given offset, starts with another string.
 * @param a String to check.
 * @param pos Offset within `a` to start checking from.
 * @param b Prefix to look for.
 * @return True if `a[pos:]` starts with `b`.
 */
bool starts_with(const std::string &a, int pos, std::string b);

/**
 * @brief Decode a backslash escape sequence at a given index.
 * @param string String containing the escape sequence (starting at the backslash).
 * @param i Index to decode from; advanced past the consumed escape sequence on success.
 * @param value Set to the decoded character/value on success.
 * @return True if a valid escape sequence was decoded.
 */
bool decode_escape_seq(const std::string &string, int &i, uint64_t &value);

/**
 * @brief Determine the numeric base a character prefix denotes (e.g. '`x`' -> 16, '`b`' -> 2).
 * @param ch Base-specifier character.
 * @param base Set to the corresponding base on success.
 * @return True if `ch` is a recognised base specifier.
 */
bool base_char(char ch, uint8_t &base);

/**
 * @brief Check whether a character is a valid digit in a given base.
 * @param c Character to check.
 * @param base Numeric base (e.g. 16 for hex).
 * @return True if `c` is a valid digit in `base`.
 */
inline bool is_base_char(char c, uint8_t base) {
  return (c >= '0' && c <= '0' + (base > 9 ? 9 : base))
         || base > 10 && (
           (c >= 'A' && c <= 'A' + (base - 10))
           || (c >= 'a' && c <= 'a' + (base - 10))
         );
}

/**
 * @brief Get the base-10 value of a digit character in a given base.
 * @param c Digit character.
 * @param base Numeric base the digit is from.
 * @return The digit's base-10 value.
 */
inline int get_base_value(char c, uint8_t base) {
  return c >= 'a' ? (c - 'a' + 10) : (c >= 'A' ? c - 'A' + 10 : (c >= '0' ? c - '0' : 0));
}

/**
 * @brief Parse a numeric literal (integer or floating-point) starting at a given index.
 * @param string String containing the literal.
 * @param index Index to parse from; advanced past the consumed literal on success.
 * @param value Set to the parsed value's bit pattern on success.
 * @param is_double Set to true if the literal was parsed as floating-point, false if integer.
 * @return True if a valid numeric literal was parsed.
 */
bool parse_number(const std::string &string, int &index, uint64_t &value, bool &is_double);

/**
 * @brief Advance an index past a label (`[A-Za-z_][0-9A-Za-z_]*`).
 * @param s String being scanned.
 * @param i Index to advance; left just past the label.
 */
void skip_label(const std::string &s, int &i);

/**
 * @brief Check whether a string is a valid label name (`[A-Za-z_][0-9A-Za-z_]*`).
 * @param label String to check.
 * @return True if `label` is a valid label name.
 */
bool is_valid_label_name(const std::string &label);

/**
 * @brief Format a value as a hexadecimal string of a fixed byte width.
 * @param value Value to format.
 * @param size_bytes Number of bytes to format (controls the digit count/zero-padding).
 * @return The formatted hex string.
 */
std::string to_hex_string(uint64_t value, uint8_t size_bytes);

/**
 * @brief Clamp a value to `[min, max)`, in place, optionally wrapping around instead of saturating.
 * @tparam T Type of the value being clamped.
 * @param value Value to clamp.
 * @param min Inclusive lower bound.
 * @param max Exclusive upper bound.
 * @param max_diff Offset subtracted from `max` when clamping/wrapping at the upper bound.
 * @param wrap_around If true, wrap past a bound to the opposite bound instead of saturating.
 */
template<typename T>
inline void clamp(T &value, T min, T max, T max_diff = 1, bool wrap_around = false) {
    if (value < min) value = wrap_around ? max - max_diff : min;
    else if (value >= max) value = wrap_around ? min : max - max_diff;
}

/**
 * @brief Apply a function to every element of a container, producing a (possibly differently-typed) container of results.
 *
 * Courtesy of http://derekwyatt.org/2011/07/15/functional-map-in-c/
 * @tparam InType Element type of the input container.
 * @tparam InContainer Template of the input container.
 * @tparam OutContainer Template of the output container (defaults to the same template as the input).
 * @tparam OutType Element type of the output container (defaults to `InType`).
 * @param input Container to map over.
 * @param func Called with each input element to produce the corresponding output element.
 * @return A new container of mapped results.
 */
template <typename InType,
        template <typename U>
        class InContainer,
        template <typename V>
        class OutContainer = InContainer,
        typename OutType = InType>
OutContainer<OutType> map(InContainer<InType>& input,
                           std::function<OutType(InType&)> func) {
    OutContainer<OutType> output;
    output.resize(input.size());
    transform(input.begin(), input.end(), output.begin(), func);
    return output;
}

/**
 * @brief Reset a string-stream to empty and clear its error/position state.
 * @tparam T Stream type (expected to support `seekp`/`seekg`/`str`/`clear`, e.g. `std::stringstream`).
 * @param s Stream to empty.
 */
template<typename T>
void empty_stream(T &s) {
  s.seekp(0);
  s.seekg(0);
  s.str("");
  s.clear();
}

/**
 * @brief Join a vector of strings with a delimiter between each.
 * @param items Strings to join.
 * @param delim Delimiter inserted between consecutive items.
 * @return The joined string.
 */
std::string join(const std::vector<std::string>& items, const std::string& delim);

/**
 * @brief Split a string on a delimiter character, calling a function on each substring.
 * @param str String to split.
 * @param delimiter Character to split on.
 * @param f Called with each substring between delimiters, in order.
 */
void split_string(const std::string& str, char delimiter, std::function<void(const std::string&)> f);

/** @brief Play a terminal bell sound. */
void bell_sound();

/**
 * @brief Combine two hash values into one.
 *
 * StackOverflow: https://stackoverflow.com/questions/5889238/why-is-xor-the-default-way-to-combine-hashes/27952689#27952689
 * @param lhs First hash value.
 * @param rhs Second hash value.
 * @return Combined hash value.
 */
size_t hash_combine(size_t lhs, size_t rhs);
