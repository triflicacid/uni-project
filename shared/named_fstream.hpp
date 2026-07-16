#pragma once

#include <optional>
#include <fstream>
#include <memory>
#include <filesystem>

/**
 * @brief A file stream paired with the path it was opened from, optionally owning that stream.
 *
 * Exists so code that receives a stream can still report which file it came from. May either
 * own the underlying `std::fstream` (constructed from a `unique_ptr`) or merely reference one
 * owned elsewhere (constructed from a `std::fstream&`), tracked by whether `ptr` is null.
 */
class named_fstream {
public:
    std::fstream &stream; ///< The wrapped stream. References `*ptr` when owned, or the caller's stream otherwise.
    std::filesystem::path path; ///< Path `stream` was opened from.

private:
    std::unique_ptr<std::fstream> ptr; ///< Owned stream, or nullptr if this instance only references one owned elsewhere.

public:
    /**
     * @brief Construct, taking ownership of the given stream.
     * @param path Path the stream was opened from.
     * @param ptr Owned stream.
     */
    named_fstream(std::filesystem::path path, std::unique_ptr<std::fstream> ptr) : path(std::move(path)), stream(*ptr), ptr(std::move(ptr)) {}

    /**
     * @brief Construct, referencing a stream owned elsewhere.
     * @param path Path the stream was opened from.
     * @param stream Stream to reference (not owned by this instance).
     */
    named_fstream(std::filesystem::path path, std::fstream &stream) : path(std::move(path)), stream(stream), ptr(nullptr) {}

    /** @brief Replace `path` with its canonical (fully resolved) form. */
    void canonicalise() {
      path = std::filesystem::canonical(path);
    }

    /** @brief Release ownership of the underlying stream to the caller. @return The owned stream, or nullptr if this instance didn't own one. */
    std::unique_ptr<std::fstream> take() {
      return std::move(ptr);
    }

    /**
     * @brief Open a file and wrap it in a `named_fstream`.
     * @param path Path to open.
     * @param mode File open mode.
     * @return A new owning `named_fstream`, or nullptr if the file could not be opened.
     */
    static std::unique_ptr<named_fstream> open(const std::filesystem::path &path, std::ios::openmode mode) {
        if (auto stream = std::make_unique<std::fstream>(path, mode); stream->is_open()) {
            return std::make_unique<named_fstream>(path, std::move(stream));
        }

        return nullptr;
    }
};
