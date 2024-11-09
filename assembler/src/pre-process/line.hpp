#pragma once

#include <iostream>
#include <string>

namespace assembler {
    struct Line {
        int n;
        std::string data;

        void print() const {
            std::cout << n << " | " << data << "\n";
        }
    };
}
