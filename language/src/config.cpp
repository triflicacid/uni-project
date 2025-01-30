#include "config.hpp"

bool lang::conf::debug = false;

bool lang::conf::lint = true;

message::Level lang::conf::lint_level = message::Warning;

std::string lang::conf::bools::true_string = "true";
uint8_t lang::conf::bools::true_value = 1;
std::string lang::conf::bools::false_string = "false";
uint8_t lang::conf::bools::false_value = 0;
