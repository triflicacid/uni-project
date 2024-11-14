#pragma once

#include <cstdint>
#include <array>

namespace processor {
    class dram {
    public:
        // DRAM size - 1MiB
        static constexpr uint64_t size = 1024 * 1024;

    private:
        std::array<uint8_t, size> mem;

    public:
        dram() = default;

        // get pointer to base data
        uint8_t *data() { return mem.data(); }

        // load a word of given size from memory
        [[nodiscard]] uint64_t load(uint64_t addr, uint8_t bytes) const;

        // store size bits of fata at addr
        void store(uint64_t addr, uint8_t bytes, uint64_t value);

        // clear DRAM memory
        void clear();

        uint8_t& operator[](std::size_t index) {
            return mem[index];
        }

        const uint8_t& operator[](std::size_t index) const {
            return mem[index];
        }
    };
}
