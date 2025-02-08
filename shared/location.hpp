#pragma once

#include <string>
#include <filesystem>
#include <utility>

class Location {
    std::filesystem::path m_path;
    int m_line;
    int m_col;

public:
    explicit Location(std::filesystem::path path, int line = -1, int col = -1) : m_path(std::move(path)), m_line(line), m_col(col) {}

    [[nodiscard]] int line() const { return m_line; }

    int &lineref() { return m_line; }

    Location &line(int n) { m_line = n; return *this; }

    [[nodiscard]] int column() const { return m_col; }

    int &columnref() { return m_col; }

    int &columnref(int n) { return m_col = n; }

    Location &column(int n) { m_col = n; return *this; }

    [[nodiscard]] const std::filesystem::path &path() const { return m_path; }

    [[nodiscard]] Location copy() const { return {*this}; }

    std::ostream &print(std::ostream &os, bool canonicalise = false) const {
        if (canonicalise) os << weakly_canonical(m_path).string();
        else os << m_path.string();

        if (m_line > -1) {
            os << ":" << m_line;
            if (m_col > -1) os << ":" << m_col;
        }

        return os;
    }

    friend std::ostream& operator<<(std::ostream& os, const Location& loc) {
        return loc.print(os);
    }
};
