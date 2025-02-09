#include "config.hpp"

bool lang::conf::debug = false;

bool lang::conf::lint = true;

message::Level lang::conf::lint_level = message::Warning;

bool lang::conf::func_decl_generate_shell = true;

std::string lang::conf::bools::true_string = "true";
std::string lang::conf::bools::false_string = "false";
