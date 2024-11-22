#include "processor.hpp"

::processor::CPU visualiser::processor::cpu;
std::unique_ptr<named_fstream> visualiser::processor::source = nullptr;
uint64_t visualiser::processor::initial_pc = 0;
std::ostringstream visualiser::processor::debug_stream;

void visualiser::processor::init() {
    ::processor::read_binary_file(cpu, source->stream);
    initial_pc = cpu.read_pc();
    cpu.ds = &debug_stream;
}

uint64_t visualiser::processor::restore_pc() {
    cpu.write_pc(initial_pc);
    return initial_pc;
}
