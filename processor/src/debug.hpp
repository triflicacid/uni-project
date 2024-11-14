#pragma once

namespace processor {
    struct Debug {
        bool cpu = false;
        bool mem = false;
        bool zflag = false;
        bool conditionals = false;
        bool errs = false;

        void set_all(bool b) {
            cpu = mem = zflag = conditionals = errs = b;
        }
    };
}
