#pragma once

/** @brief A stream buffer that discards everything written to it, backing @ref nullstream. */
class nullbuf : public std::streambuf {
    char buffer[100];

    /**
     * @brief Discard an overflowed character by resetting the put area, without ever flushing it anywhere.
     * @param c Character that triggered the overflow.
     * @return `c`, unchanged, to signal success.
     */
    int overflow(int c) override {
        setp(buffer, buffer + sizeof buffer);
        return c;
    }

    /**
     * @brief Discard a block write.
     * @param n Number of characters that would have been written.
     * @return `n`, to signal all characters were "written".
     */
    std::streamsize xsputn(const char*, std::streamsize n) override {
        return n;
    }
};

/** @brief An output stream that silently discards everything written to it (a "/dev/null" stream). */
class nullstream : public std::ostream {
public:
    /** @brief Construct a discarding stream. */
    nullstream() : std::ostream(nullptr) {}
    /** @brief Copy-construct. The copy does not read from the source; it is simply a new, independent discarding stream. */
    nullstream(const nullstream &) : std::ostream(nullptr) {}
};

/**
 * @brief Discard a value streamed to a `nullstream`.
 * @tparam T Type of the streamed value.
 * @param os Discarding stream being streamed to.
 * @param value Value to discard.
 * @return `os`, for chaining.
 */
template <class T>
const nullstream &operator<<(nullstream &&os, const T &value) {
    return os;
}