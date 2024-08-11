#pragma once

namespace assembler {
  struct Label {
    int line;
    int col;
    long long addr;
  };
}
