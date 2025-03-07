#include "util.hpp"

#include <utility>
#include <iostream>

std::string &ltrim(std::string &s, const char *t) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

std::string &rtrim(std::string &s, const char *t) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

std::string &trim(std::string &s, const char *t) {
  return ltrim(rtrim(s, t), t);
}

void skip_whitespace(const std::string &s, int &i) {
  size_t len = s.length();
  while (i < len && s[i] == ' ')
    i++;
}

void skip_non_whitespace(const std::string &s, int &i) {
  size_t len = s.length();
  while (i < len && s[i] != ' ')
    i++;
}

void skip_to_break(const std::string &s, int &i) {
  size_t len = s.length();
  while (i < len && s[i] != ',' && s[i] != ' ' && s[i] != ')' && s[i] != ']')
    i++;
}

void skip_alpha(const std::string &s, int &i) {
  size_t len = s.length();
  while (i < len && std::isalpha(s[i]))
    i++;
}

void skip_alphanum(const std::string &s, int &i) {
  size_t len = s.length();
  while (i < len && std::isalnum(s[i]))
    i++;
}

bool starts_with(const std::string &a, std::string b) {
  return starts_with(a, 0, std::move(b));
}

bool starts_with(const std::string &a, int pos, std::string b) {
  if (a.length() < b.length()) {
    return false;
  }

  return a.substr(pos, b.length()) == b;
}

bool decode_escape_seq(const std::string &string, int &i, uint64_t &value) {
  switch (string[i]) {
    case 'b': // BACKSPACE
      i++;
      value = 0x8;
      break;
    case 'n': // NEWLINE
      i++;
      value = 0xA;
      break;
    case 'r': // CARRIAGE RETURN
      i++;
      value = 0xD;
      break;
    case 's': // SPACE
      i++;
      value = 0x20;
      break;
    case 't': // HORIZONTAL TAB
      i++;
      value = 0x9;
      break;
    case 'v': // VERTICAL TAB
      i++;
      value = 0xB;
      break;
    case '0': // NULL
      i++;
      value = 0x0;
      break;
    case 'd': {
      // DECIMAL SEQUENCE
      int len = 0, start = ++i;
      uint32_t k = 1;
      value = 0;

      while (i < string.size() && string[i] >= '0' && string[i] <= '9') {
        if (i++ != start) k *= 10;
        ++len;
      }

      for (int j = 0; j < len; ++j, k /= 10) {
        value += (string[start + j] - '0') * k;
      }

      break;
    }
    case 'o': {
      // OCTAL SEQUENCE
      int len = 0, start = ++i;
      uint32_t k = 1;
      value = 0;

      while (i < string.size() && string[i] >= '0' && string[i] < '8') {
        if (i++ != start) k *= 8;
        ++len;
      }

      for (int j = 0; j < len; ++j, k /= 8) {
        value += (string[start + j] - '0') * k;
      }

      break;
    }
    case 'x': {
      // HEXADECIMAL SEQUENCE
      int len = 0, start = ++i;
      uint32_t k = 1;
      value = 0;

      while (i < string.size() && is_base_char(string[i], 16)) {
        if (i++ != start) k *= 16;
        ++len;
      }

      for (int j = 0; j < len; ++j, k /= 16) {
        value += get_base_value(string[start + j], 16) * k;
      }

      break;
    }
    default: // Unknown
      return false;
  }

  return true;
}

bool base_char(char ch, uint8_t &base) {
  switch (ch) {
    case 'b':
      base = 2;
      return true;
    case 't':
      base = 3;
      return true;
    case 'd':
      base = 10;
      return true;
    case 'x':
      base = 16;
      return true;
    case 'o':
      base = 8;
      return true;
    default:
      return false;
  }
}

bool parse_number(const std::string &string, int &index, uint64_t &value, bool &is_double) {
  value = 0;
  uint8_t base = 10;
  double decimal_portion = 0; // decimal portion of double
  is_double = false;
  int decimal_denom = 1; // denominator value of float calculation
  bool neg = false;
  uint8_t digit_count = 0;

  // is the number negative?
  if (string[index] == '-') {
    neg = true;
    index++;
  }

  // does the number start with a base indicator?
  if (string[index] == '0') {
    index++;

    if (base_char(string[index], base)) {
      index++;
    } else if (string[index] == '0') {
      return false;
    } else {
      digit_count++;
    }
  }

  // parse and calculate value
  for (; index < string.length(); index++) {
    if (string[index] == '.') {
      if (is_double) {
        break;
      }

      // found double!
      is_double = true;
      decimal_denom = base;
    } else if (string[index] == '_') {
    } else if (is_base_char(string[index], base)) {
      int char_value = get_base_value(string[index], base);

      if (is_double) {
        decimal_portion += (double) char_value / decimal_denom;
        decimal_denom *= base;
      } else {
        value = value * base + char_value;
      }

      digit_count++;
    } else {
      break;
    }
  }

  // check that we encountered some digits
  if (digit_count == 0) {
    return false;
  }

  // negate value if required
  // also: if double, add dp portion and transfer over
  if (is_double) {
    decimal_portion += (double) value;
    if (neg) decimal_portion *= -1;
    value = *(uint64_t *) &decimal_portion;
  } else if (neg) {
    value *= -1;
  }

  return true;
}

bool is_valid_label_name(const std::string &label) {
  if (label.empty()) return false;

  if (!std::isalpha(label[0]) && label[0] != '_') return false;

  for (int i = 1; i < label.size(); ++i) {
    if (!std::isalnum(label[i]) && label[i] != '_') {
      return false;
    }
  }

  return true;
}

std::string to_hex_string(uint64_t value, uint8_t size_bytes) {
  std::stringstream s;
  s << std::setw(size_bytes * 2) << std::setfill('0');
  s << std::hex << value;
  return s.str();
}

std::string join(const std::vector<std::string>& items, const std::string& delim) {
  std::stringstream s;
  for (const std::string& item : items)
    s << item << delim;
  return s.str();
}

void split_string(const std::string &str, char delimiter, std::function<void(const std::string&)> f) {
  size_t from = 0;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == delimiter) {
      f(str.substr(from, i - from));
      from = i + 1;
    }
  }

  if (from <= str.size())
    f(str.substr(from));
}

void bell_sound() {
  std::cout << "\x1b[\007";
}

size_t hash_combine(size_t lhs, size_t rhs) {
  if constexpr (sizeof(size_t) >= 8) {
    lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
  } else {
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
  }
  return lhs;
}
