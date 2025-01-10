#pragma once

#include <optional>
#include <fstream>
#include <memory>
#include <filesystem>

class named_fstream {
public:
    std::fstream &stream;
    std::filesystem::path path;

private:
    std::unique_ptr<std::fstream> ptr; // may be nullptr, in which case we do not have ownership, just a reference

public:
    named_fstream(std::filesystem::path path, std::unique_ptr<std::fstream> ptr) : path(std::move(path)), stream(*ptr), ptr(std::move(ptr)) {}

    named_fstream(std::filesystem::path path, std::fstream &stream) : path(std::move(path)), stream(stream), ptr(nullptr) {}

    void canonicalise() {
      path = std::filesystem::canonical(path);
    }

    static std::unique_ptr<named_fstream> open(const std::filesystem::path &path, std::ios::openmode mode) {
        if (auto stream = std::make_unique<std::fstream>(path, mode); stream->is_open()) {
            return std::make_unique<named_fstream>(path, std::move(stream));
        }

        return nullptr;
    }
};
