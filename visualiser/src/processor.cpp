#include "processor.hpp"

::processor::CPU visualiser::processor::cpu;
std::unique_ptr<named_fstream> visualiser::processor::source = nullptr;
std::ostringstream visualiser::processor::debug_stream;
