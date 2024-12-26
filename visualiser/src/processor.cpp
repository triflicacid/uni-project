#include "processor.hpp"

::processor::CPU visualiser::processor::cpu;
std::unique_ptr<named_fstream> visualiser::processor::source = nullptr;
uint64_t visualiser::processor::initial_pc = 0;
std::unique_ptr<named_fstream> visualiser::processor::piped_stdout;
std::unique_ptr<named_fstream> visualiser::processor::piped_stdin;

void visualiser::processor::init() {
  ::processor::read_binary_file(cpu, source->stream);
  initial_pc = cpu.read_pc();
  ::processor::debug::args = true;
  ::processor::debug::set_all(true);
  if (piped_stdin) cpu.is = &piped_stdin->stream;
  if (piped_stdout) cpu.os = &piped_stdout->stream;
}

uint64_t visualiser::processor::restore_pc() {
  cpu.write_pc(initial_pc);
  return initial_pc;
}
