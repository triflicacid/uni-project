#include "debug.hpp"

bool processor::debug::cpu = false;
bool processor::debug::mem = false;
bool processor::debug::reg = false;
bool processor::debug::zflag = false;
bool processor::debug::conditionals = false;
bool processor::debug::errs = false;

void processor::debug::set_all(bool b) {
    cpu = mem = reg = zflag = conditionals = errs = b;
}