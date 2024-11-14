#pragma once

namespace processor::debug {
    extern bool cpu;
    extern bool mem;
    extern bool reg;
    extern bool zflag;
    extern bool conditionals;
    extern bool errs;

    void set_all(bool b);
}
