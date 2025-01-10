#pragma once

namespace assembler {
  struct Label {
    Location loc;
    uint64_t addr = 0;
  };
}
