#pragma once

class nullbuf : public std::streambuf {
    char buffer[100];

    int overflow(int c) override {
        setp(buffer, buffer + sizeof buffer);
        return c;
    }

    std::streamsize xsputn(const char*, std::streamsize n) override {
        return n;
    }
};

class nullstream : public std::ostream {
public:
    nullstream() : std::ostream(nullptr) {}
    nullstream(const nullstream &) : std::ostream(nullptr) {}
};

template <class T>
const nullstream &operator<<(nullstream &&os, const T &value) {
    return os;
}