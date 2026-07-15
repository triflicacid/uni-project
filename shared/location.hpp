#pragma once

#include <string>
#include <filesystem>
#include <utility>

/**
 * @brief A source location: a file path with an optional line and column, used to attribute diagnostics to source text.
 *
 * A line/column of -1 represents "not known"/"not applicable" rather than a real position.
 */
class Location {
    std::filesystem::path m_path; ///< Source file path.
    int m_line; ///< 1-based line number, or -1 if not known.
    int m_col; ///< 1-based column number, or -1 if not known.

public:
    /**
     * @brief Construct a location.
     * @param path Source file path.
     * @param line 1-based line number, or -1 if not known.
     * @param col 1-based column number, or -1 if not known.
     */
    explicit Location(std::filesystem::path path, int line = -1, int col = -1) : m_path(std::move(path)), m_line(line), m_col(col) {}

    /** @brief Get the line number. @return Line number, or -1 if not known. */
    [[nodiscard]] int line() const { return m_line; }

    /** @brief Get a mutable reference to the line number. @return Reference to the line field. */
    int &lineref() { return m_line; }

    /**
     * @brief Set the line number.
     * @param n New line number.
     * @return `*this`, for chaining.
     */
    Location &line(int n) { m_line = n; return *this; }

    /** @brief Get the column number. @return Column number, or -1 if not known. */
    [[nodiscard]] int column() const { return m_col; }

    /** @brief Get a mutable reference to the column number. @return Reference to the column field. */
    int &columnref() { return m_col; }

    /**
     * @brief Set the column number, returning a reference to the stored field.
     * @param n New column number.
     * @return Reference to the column field.
     */
    int &columnref(int n) { return m_col = n; }

    /**
     * @brief Set the column number.
     * @param n New column number.
     * @return `*this`, for chaining.
     */
    Location &column(int n) { m_col = n; return *this; }

    /** @brief Get the source file path. @return The path. */
    [[nodiscard]] const std::filesystem::path &path() const { return m_path; }

    /** @brief Copy this location. @return A new `Location` with the same path/line/column. */
    [[nodiscard]] Location copy() const { return {*this}; }

    /**
     * @brief Write this location to a stream as `path[:line[:col]]`.
     * @param os Stream to write to.
     * @param canonicalise If true, write the path in canonical (fully resolved) form instead of as-is.
     * @return `os`, for chaining.
     */
    std::ostream &print(std::ostream &os, bool canonicalise = false) const {
        if (canonicalise) os << weakly_canonical(m_path).string();
        else os << m_path.string();

        if (m_line > -1) {
            os << ":" << m_line;
            if (m_col > -1) os << ":" << m_col;
        }

        return os;
    }

    /**
     * @brief Write a location to a stream via @ref print.
     * @param os Stream to write to.
     * @param loc Location to write.
     * @return `os`, for chaining.
     */
    friend std::ostream& operator<<(std::ostream& os, const Location& loc) {
        return loc.print(os);
    }
};
