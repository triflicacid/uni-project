#pragma once

namespace assembler {
    struct Label {
        int line{};
        int col{};
        uint64_t addr{};
    };
}
