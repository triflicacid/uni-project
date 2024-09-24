#pragma once

namespace language::options {
  extern char default_entry_point[];
  extern bool allow_alter_entry;  // allow program's entry point to be changed
  extern bool allow_shadowing;  // allow variable shadowing
  extern bool must_declare_functions;  // functions must be declared before they are defined
  extern int unused_symbol_level;  // reporting level of "unused symbol", -1 is silent
}
