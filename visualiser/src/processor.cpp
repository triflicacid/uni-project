#include "processor.hpp"

::processor::CPU visualiser::processor::cpu;
std::unique_ptr<named_fstream> visualiser::processor::source = nullptr;
uint64_t visualiser::processor::initial_pc = 0;
std::unique_ptr<named_fstream> visualiser::processor::piped_stdout;
std::unique_ptr<named_fstream> visualiser::processor::piped_stdin;
std::set<const visualiser::sources::PCLine*> visualiser::processor::breakpoints;
uint64_t visualiser::processor::pc = 0;
const visualiser::sources::PCLine* visualiser::processor::pc_line = nullptr;

void visualiser::processor::init() {
  cpu.reset();
  ::processor::read_binary_file(cpu, source->stream);
  initial_pc = cpu.read_pc();
  //::processor::debug::args = true;
  //::processor::debug::set_all(true);
  if (piped_stdin) cpu.is = &piped_stdin->stream;
  if (piped_stdout) cpu.os = &piped_stdout->stream;
  update_pc(0);
}

uint64_t visualiser::processor::restore_pc() {
  cpu.write_pc(initial_pc);
  return initial_pc;
}

void visualiser::processor::update_pc() {
  pc = visualiser::processor::cpu.read_pc();
  if (const sources::PCLine* new_pc_line = visualiser::sources::locate_pc(pc))
    pc_line = new_pc_line;
}

void visualiser::processor::update_pc(uint64_t val) {
  pc = val;
  cpu.write_pc(val);
  if (const sources::PCLine* new_pc_line = visualiser::sources::locate_pc(val))
    pc_line = new_pc_line;
}
