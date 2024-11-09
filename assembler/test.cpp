#include <cstdint>
#include <iostream>
#include <string>
#include <util.hpp>

int main() {
    std::string str = "1, 2";
    int i = 0;
    bool is_double;
    uint64_t value;

    std::cout << "string: '" << str << "'\n";
    bool ok = parse_number(str, i, value, is_double);

    if (ok) {
        std::cout << "is double = " << is_double << "\n";
        std::cout << "value = ";

        if (is_double) {
            std::cout << *(double *) &value;
        } else {
            std::cout << value;
        }

        std::cout << "\nindex = " << i << "\n";
    } else {
        std::cout << "failure\n";
    }
    std::cout << "remainder: '" << str.substr(i) << "'\n";

    return EXIT_SUCCESS;
}
