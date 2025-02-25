#include "config.hpp"

bool lang::conf::debug = false;

bool lang::conf::lint = true;

message::Level lang::conf::lint_level = message::Warning;

bool lang::conf::function_placeholder = false;

bool lang::conf::always_define_symbols = false;

bool lang::conf::indent_asm_code = true;

std::string lang::conf::bools::true_string = "true";
std::string lang::conf::bools::false_string = "false";
