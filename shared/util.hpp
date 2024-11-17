#pragma once

#include <string>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>

/** Trim string from the left. */
std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v");

/** Trim string from the right. */
std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v");

/** Trim string from the left and the right. */
std::string &trim(std::string &s, const char *t = " \t\n\r\f\v");

/** Transform string to lowercase. */
inline void to_lowercase(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

/** Advance past whitespace, increment index */
void skip_whitespace(const std::string &s, int &i);

/** Advance past non-whitespace characters, increment index */
void skip_non_whitespace(const std::string &s, int &i);

/** Advance past non-break characters: whitespace or comma. */
void skip_to_break(const std::string &s, int &i);

/** Advance past alpha characters, increment index */
void skip_alpha(const std::string &s, int &i);

/** Advance past alphanumeric characters, increment index */
void skip_alphanum(const std::string &s, int &i);

/** Return whether string a starts with string b */
bool starts_with(const std::string &a, std::string b);

/** Return whether string a[idx:] starts with string b */
bool starts_with(const std::string &a, int pos, std::string b);

/** Decode escape sequence '\...'. Return if OK.  */
bool decode_escape_seq(const std::string &string, int &i, uint64_t &value);

/** gGiven character, populate base. Return success. */
bool base_char(char ch, uint8_t &base);

/** Return whether the given character is within the given base. */
inline bool is_base_char(char c, uint8_t base) {
  return (c >= '0' && c <= '0' + (base > 9 ? 9 : base))
         || base > 10 && (
           (c >= 'A' && c <= 'A' + (base - 10))
           || (c >= 'a' && c <= 'a' + (base - 10))
         );
}

/** Given a character in the given base, return base10 value. */
inline int get_base_value(char c, uint8_t base) {
  return c >= 'a' ? (c - 'a' + 10) : (c >= 'A' ? c - 'A' + 10 : (c >= '0' ? c - '0' : 0));
}

/** Convert integer string to decimal integer/float, return success. */
bool parse_number(const std::string &string, int &index, uint64_t &value, bool &is_double);

/** Return is valid label name: [A-Za-z][0-9A-Za-z]* */
bool is_valid_label_name(const std::string &label);

/** convert number to hex */
template<typename T>
std::string to_hex_string(T value) {
    std::stringstream s;
    s << std::setw(sizeof(T) * 2) << std::setfill('0');
    s << std::hex << value;
    return s.str();
}
