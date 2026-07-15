Architecture Overview {#architecture_overview}
=========================

A high-level tour of how the four components fit together, how each one is internally staged, and two of the
compiler's central mechanisms: the Edel type system and the function call protocol. This is structural context
only; for per-class and per-method detail, see the generated reference itself (Namespaces, Classes, Files
tabs).

## System Overview

The project implements a complete, miniature code-execution toolchain, built as four independent CMake
targets sharing one `shared/` library.

\dot
digraph system_overview {
  rankdir=LR;
  fontname="Helvetica";
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, fillcolor="#2b6cb0", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  edel [label="program.edel\n(Edel source)"];
  asmtxt [label="program.asm\n(target assembly text)"];
  recon [label="program.s\n(reconstructed assembly,\n1:1 with the binary)"];
  bin [label="program\n(machine-code binary)", fillcolor="#6b46c1"];
  exec [label="stdout / register\n& memory state", shape=ellipse, fillcolor="#718096"];
  vis [label="visualiser\n(embeds the processor directly;\nties all four artifacts together)", shape=box, style="rounded,filled,dashed", fillcolor="#805ad5"];

  edel -> asmtxt [label="compiler/"];
  asmtxt -> bin [label="assembler/"];
  asmtxt -> recon [label="assembler/ -r"];
  bin -> exec [label="processor/"];

  { rank=same; edel; asmtxt; recon; bin; }
  edel -> vis [style=dotted, dir=none];
  asmtxt -> vis [style=dotted, dir=none];
  recon -> vis [style=dotted, dir=none];
  bin -> vis [style=dotted, dir=none];
}
\enddot

Each stage is a separate, independently runnable executable: there is no in-process pipeline (the compiler
never calls the assembler; `compile.sh` just shells out to each binary in turn). This keeps the four programs
decoupled, testable command-line tools with plain-text/binary file interfaces between them, mirroring a real
toolchain (`cc` -> `as` -> executable) and making each stage independently inspectable, in keeping with the
project's aim as an educational toolchain.

All four targets are declared from the single top-level `CMakeLists.txt`, requiring C++20 and building into a
shared `out/` directory. Only the visualiser has an external dependency (FTXUI, fetched via `FetchContent`);
the other three are dependency-free, built purely from `shared/` plus their own `src/`.

## The Compiler (Edel -> assembly)

The compiler translates a small statically-typed language called Edel into the project's custom target
assembly. It is the largest and most architecturally rich component.

### Pipeline

\dot
digraph compiler_pipeline {
  rankdir=TB;
  fontname="Helvetica";
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, fillcolor="#718096", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  lex [label="lexer::Lexer"];
  parse [label="parser::Parser::parse()"];
  ast [label="ast::ProgramNode\n(single AST, built once)"];
  p1 [label="Phase 1: collate_registry\n(gather forward-referenceable symbols)", fillcolor="#2b6cb0"];
  p2 [label="Phase 2: process\n(type-check, build value::Value)", fillcolor="#2b6cb0"];
  p3 [label="Phase 3: resolve\n(disambiguate overloads; no code emitted)", fillcolor="#2b6cb0"];
  p4 [label="Phase 4: generate_code\n(emits assembly::Program)", fillcolor="#6b46c1"];
  out [label="Program::print()\n(assembly text: stdout / file)"];

  lex -> parse -> ast -> p1 -> p2 -> p3 -> p4 -> out;
}
\enddot

Phases 1 to 3 are pure semantic analysis and emit no code; phase 4 is the only phase that touches
`assembly::Program`. There is one AST, built once, and it is walked multiple times, once per phase, via
virtual methods directly on `ast::Node` rather than a separate visitor class. Each concrete node overrides
only the phases it needs:

- **collate_registry** declares this node's own symbols into a scope-local registry so siblings can
  forward-reference them (for example, calling a function declared later in the same block).
- **process** (pure virtual, every node must implement it) type-checks the node and builds its
  `value::Value`. It must not emit any code and must not call phase-4 methods on children.
- **resolve** is a no-op by default; used for anything that needs full-program context to disambiguate,
  chiefly resolving an overload set once a type hint becomes available, but still must not emit code.
- **generate_code** emits assembly into the shared `Context` by manipulating the register allocator and
  stack manager.

This split exists so that forward references (calling a function defined later in the file, or referencing an
overload set only later disambiguated) are possible while preserving the invariant that no code is generated
until the entire program is known to type-check. There is no separate intermediate representation and no
optimisation or peephole pass anywhere in the pipeline: phase 4 emits target assembly lines directly from AST
nodes, so code quality is a direct function of how each node's `generate_code` was written. Everything is
driven from `main.cpp`; a single mutable `Context` (message list, `assembly::Program`, `StackManager`,
`RegisterAllocationManager`, `SymbolTable`, and a loop-context stack) is constructed once and threaded by
reference through virtually every AST call.

### Edel Type System

Every type is a `type::Node` with a globally unique `TypeId`, a byte size, a label used for name-mangling, and
a mapping to the processor's native scalar datatype. Edel has sized integers (`u8`/`i8` through `u64`/`i64`,
with `byte`/`int`/`long` aliases), floats (`f32`/`f64`, aliased `float`/`double`), `bool`, the zero-sized unit
type `()`, pointers `*T`, fixed-size arrays `[T; n]`, function types, and namespaces. Coercion between the
numeric types follows a strict, widening-only lattice:

\dot
digraph type_lattice {
  rankdir=LR;
  fontname="Helvetica";
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, fillcolor="#2b6cb0", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  bool [fillcolor="#718096"];

  u8 -> u16 -> u32 -> u64;
  i8 -> i16 -> i32 -> i64;
  u8 -> i16;
  u16 -> i32;
  u32 -> i64;

  f32 [fillcolor="#6b46c1"];
  f64 [fillcolor="#6b46c1"];
  u64 -> f32 [label="(all integer types)"];
  i64 -> f32;
  f32 -> f64;

  subgraph cluster_outside {
    label="Outside the coercion lattice";
    style=dashed; fontname="Helvetica"; fontsize=10; fontcolor="#718096";
    ptr [label="*T\n(pointer)", fillcolor="#805ad5"];
    arr [label="[T; n]\n(array)", fillcolor="#805ad5"];
    unit [label="()\n(unit)", fillcolor="#805ad5"];
    arr -> ptr [label="decays to", style=dashed];
  }
}
\enddot

An edge means "coerces to": `u8` widens into `u16`, an unsigned type widens into a strictly larger signed
type, and every integer type widens into both float types. `bool` coerces to nothing.

Pointers, arrays, and the unit type sit outside this lattice entirely; none of them are `add_subtype`d into the
graph, so `is_subtype` never returns true for them. Each has its own, separately defined rules instead: `*T`
and `[T; n]` are structural wrapper types (a `PointerNode`/`ArrayNode` around an inner `Node`, one per distinct
`T`, so `*int` and `*bool` are different `TypeId`s), and `[T; n]` decays into `*T` (referencing an array yields
a pointer to its first element) rather than coercing into it. `()` is zero-sized and carries no assembly
datatype at all: `UnitNode::get_asm_datatype()` always throws, since code generation should never need to ask.
Function types and namespaces are likewise outside the lattice but are omitted from the diagram above for
clarity.

`type::graph` (a `TypeGraph` wrapping the shared generic `Graph<Key, Node>`) serves two purposes. It **interns**
every type instance ever constructed, so `*int` or `[int; 5]` always resolve to the same `TypeId`, via a
memoising factory that scans for an existing equivalent wrapper before building a new one. It also encodes the
lattice above as directed graph edges, with `is_subtype(child, parent)` implemented as graph reachability.
This one structure is consulted everywhere an implicit coercion decision is needed: return-type checks,
loop-guard checks, variable-declaration assignment checks, branch materialisation.

Overload resolution is not pure subtype-graph reachability: `FunctionNode::filter_candidates` computes, per
candidate, a similarity score (the count of exactly-matching argument types) among all subtype-compatible
candidates, keeps only the maximal-score set, and short-circuits on an exact all-argument match. Ambiguous or
empty results produce a diagnostic listing every near-miss candidate as a note.

### Function Call Process

The target is a register/stack machine with no hardware call-stack support beyond plain load/store, so the
calling convention is a hand-written, cdecl-style protocol split between caller and callee, with no formal ABI
structure: the two sides simply agree on fixed offsets.

\dot
digraph call_sequence {
  rankdir=TB;
  fontname="Helvetica";
  nodesep=0.7;
  ranksep=0.22;
  node [fontname="Helvetica", fontsize=9];
  edge [fontname="Helvetica", fontsize=9];

  // participant headers
  a0 [label="Caller\nops::call_function", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white", fontsize=10];
  b0 [label="Callee\nFunctionBaseNode::define", shape=box, style="rounded,filled", fillcolor="#718096", fontcolor="white", fontsize=10];
  { rank=same; a0; b0; }
  a0 -> b0 [style=invis];

  // caller: pre-call steps
  a1 [label="reserve return buffer\n(if large/aggregate return)", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b1 [shape=point, width=0.01, style=invis];
  { rank=same; a1; b1; }

  a2 [label="save_store(true)\nspill required registers to stack", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b2 [shape=point, width=0.01, style=invis];
  { rank=same; a2; b2; }

  a3 [label="push each argument\n(mem_copy for aggregates)", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b3 [shape=point, width=0.01, style=invis];
  { rank=same; a3; b3; }

  a4 [label="push $rpc, push $fp", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b4 [shape=point, width=0.01, style=invis];
  { rank=same; a4; b4; }

  a5 [label="push_frame(true)\n$fp := $sp (physical swap)", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b5 [shape=point, width=0.01, style=invis];
  { rank=same; a5; b5; }

  // message: jal
  a6 [shape=point, width=0.06];
  b6 [shape=point, width=0.06];
  { rank=same; a6; b6; }

  // callee: body steps
  a7 [shape=point, width=0.01, style=invis];
  b7 [label="push_frame(false)\nbookkeeping only: $fp already set", shape=box, style="rounded,filled", fillcolor="#718096", fontcolor="white"];
  { rank=same; a7; b7; }

  a8 [shape=point, width=0.01, style=invis];
  b8 [label="bind parameters at negative $fp\noffsets, generate body", shape=box, style="rounded,filled", fillcolor="#718096", fontcolor="white"];
  { rank=same; a8; b8; }

  a9 [shape=point, width=0.01, style=invis];
  b9 [label="pop_frame(false)\nbookkeeping only", shape=box, style="rounded,filled", fillcolor="#718096", fontcolor="white"];
  { rank=same; a9; b9; }

  // message: ret
  a10 [shape=point, width=0.06];
  b10 [shape=point, width=0.06];
  { rank=same; a10; b10; }

  // caller: post-call steps
  a11 [label="pop_frame(true)\n$sp := $fp, discard callee locals", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b11 [shape=point, width=0.01, style=invis];
  { rank=same; a11; b11; }

  a12 [label="reload $fp, $rpc from saved slots", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b12 [shape=point, width=0.01, style=invis];
  { rank=same; a12; b12; }

  a13 [label="destroy_store(true)\nrestore saved registers", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b13 [shape=point, width=0.01, style=invis];
  { rank=same; a13; b13; }

  a14 [label="repopulate $ret slot", shape=box, style="rounded,filled", fillcolor="#2b6cb0", fontcolor="white"];
  b14 [shape=point, width=0.01, style=invis];
  { rank=same; a14; b14; }

  // lifelines
  a0 -> a1 -> a2 -> a3 -> a4 -> a5 -> a6 -> a7 -> a8 -> a9 -> a10 -> a11 -> a12 -> a13 -> a14 [style=dashed, arrowhead=none, color="#999999"];
  b0 -> b1 -> b2 -> b3 -> b4 -> b5 -> b6 -> b7 -> b8 -> b9 -> b10 -> b11 -> b12 -> b13 -> b14 [style=dashed, arrowhead=none, color="#999999"];

  // messages between lifelines
  a6 -> b6 [label="jal <function>", color="#6b46c1", fontcolor="#6b46c1", penwidth=1.5];
  b10 -> a10 [label="ret", style=dashed, color="#718096", fontcolor="#718096"];

  // pin left-to-right column order at every rank (otherwise the "ret" edge,
  // which points right-to-left, can flip the columns below it)
  a1 -> b1 [style=invis]; a2 -> b2 [style=invis]; a3 -> b3 [style=invis];
  a4 -> b4 [style=invis]; a5 -> b5 [style=invis]; a7 -> b7 [style=invis];
  a8 -> b8 [style=invis]; a9 -> b9 [style=invis]; a10 -> b10 [style=invis];
  a11 -> b11 [style=invis]; a12 -> b12 [style=invis]; a13 -> b13 [style=invis];
  a14 -> b14 [style=invis];
}
\enddot

The caller performs the physical `$fp`/register swap; the callee only tracks the resulting stack offsets. This
is a deliberate, explicit division of responsibility hard-coded into these two call sites, not a formalised
ABI description. Parameters are bound as stack symbols at negative offsets from `$fp`, starting 16 bytes in to
skip the saved `$rpc`/`$fp` pair, walked in reverse to match push order. Locally declared variables grow the
frame forward from where parameters end (positive offsets). Overloaded operators compile to plain function
calls through this same path, with no special-cased ABI. Function definitions are lazy: a function's body is
only actually code-generated the first time it is called or addressed, so unused overloads or forward
declarations never cost any emitted code.

## The Assembler (assembly -> machine code)

The assembler turns the compiler's (or a human's) target assembly text into a raw binary the processor can
load.

\dot
digraph assembler_pipeline {
  rankdir=LR;
  fontname="Helvetica";
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, fillcolor="#718096", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  read [label="read_source_file()\n(read lines)"];
  pre [label="pre_process()\n(directives, macros,\nconstant substitution)"];
  parse [label="parser::parse()\n(grammar, labels,\ninstruction encoding)"];
  write [label="Data::write()\n(emit binary:\nheader + bytes)", fillcolor="#6b46c1"];

  read -> pre -> parse -> write;
}
\enddot

`main.cpp` wires these four stages together, calling `message::print_and_check()` after each and aborting on
the first error-level message. `-p`/`-r`/`--no-pre-process`/`--no-compile` flags let any stage's output be
inspected or skipped independently, useful both for debugging the assembler itself and, via `-r` (reconstructed
assembly), for the visualiser's source-tracing feature.

The pre-processor operates purely on text before the parser ever runs: the parser never sees a `%`-directive,
a macro, or a constant name, since everything textual is fully expanded first. Supported directives include
`%define` (textual constant substitution), `%include` (recursive, with circular-include detection), and
`%macro`/`%end` (captures a raw-line body, substituted positionally at call sites).

Label resolution is single-pass with retroactive patching, not classic two-pass assembly. When an instruction
references a label that is already known, it is resolved immediately. When it references a label that has not
been declared yet, the reference is left as a live, unresolved placeholder inside the already-emitted
instruction chunk; the moment that label is later declared, every chunk already in the buffer is walked and any
matching placeholder is patched in place. Pseudo-instructions (`b`, `jmp`, `ret`, `loadi`, `zero`, `exit`,
`int`, `rti`) carry a dummy opcode and always route through an intercept transform before reaching the
encoder, expanding into one or more real instructions.

## The Processor (machine code -> execution)

A from-scratch emulator for a custom 64-bit RISC ISA.

\dot
digraph processor_modules {
  rankdir=TB;
  fontname="Helvetica";
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, fillcolor="#2b6cb0", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  main [label="main.cpp\nCLI, I/O wiring, drives the step loop,\nprints debug trace", fillcolor="#6b46c1"];
  cpu [label="CPU (inherits Core)\nISA semantics: fetch/decode/dispatch/execute,\nflags, addressing modes, syscalls, interrupts"];
  core [label="Core\nregister file (32 x 64-bit), memory-access\nprimitives, debug-message plumbing"];
  bus [label="bus\nthin pass-through to DRAM (an intentional\nno-op seam for future middleware/MMIO)", fillcolor="#718096"];
  dram [label="dram\nflat 1 MiB byte array, raw load/store,\nno bounds checking of its own", fillcolor="#718096"];

  main -> cpu -> core -> bus -> dram;
}
\enddot

Responsibility is layered strictly: `dram` is dumb storage, `bus` is a deliberate indirection point left as a
seam for future middleware, `Core` provides register/memory access plus a debug-instrumentation hook fired on
every access, and `CPU` layers the entire instruction set on top.

Every instruction word carries a 6-bit opcode, a 4-bit conditional-test field (any instruction can be
predicated on the comparison/zero flags; there is no separate conditional-branch opcode), and, for typed
operations, one or two 3-bit datatype fields. The opcode set itself is intentionally small: everything else,
including branches, `ret`, 64-bit immediate loads, and `exit`, is a pseudo-instruction synthesised entirely in
the assembler, not the processor.

### Fetch-Decode-Execute Cycle

\dot
digraph fetch_execute {
  rankdir=TB;
  fontname="Helvetica";
  nodesep=0.35;
  ranksep=0.3;
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=9, fillcolor="#2b6cb0", fontcolor="white"];
  edge [fontname="Helvetica", fontsize=9];

  start [label="main.cpp: while (running)\ncall cpu.step(step)"];
  interrupt [label="is_interrupt()?\n!in_interrupt flag &amp;&amp; (isr &amp; imr)", shape=diamond, fillcolor="#718096"];
  handle_int [label="handle_interrupt()\nsave $pc to $ipc, set in_interrupt flag,\n$pc := addr_interrupt_handler"];
  fetch [label="fetch()\ncheck_memory($pc), then mem_load 8 bytes"];
  halted [label="is_running()?", shape=diamond, fillcolor="#718096"];
  stop [label="halted\n(e.g. raise_error(segfault)\nfrom an out-of-range $pc)", fillcolor="#4a5568"];
  advance [label="reg_set($pc, $pc + 8)\n(before execute, so a jump-style\ninstruction just overwrites $pc)"];
  nop [label="opcode == _nop?", shape=diamond, fillcolor="#718096"];
  nop_handle [label="halt() if --halt-on-nop,\notherwise skip this cycle"];
  condtest [label="conditional-test bits\nvs. $flag (cmp / zero bits,\noptional inverse-test bit)", shape=diamond, fillcolor="#718096"];
  skip [label="skip execution\n(predicate failed)", fillcolor="#a0aec0"];
  decode [label="execute(inst)\nswitch (opcode): dispatches to one\nexec_* handler per opcode", fillcolor="#6b46c1"];

  next [label="step++"];

  start -> interrupt;
  interrupt -> handle_int [label="yes"];
  interrupt -> fetch [label="no"];
  handle_int -> fetch;

  fetch -> halted;
  halted -> stop [label="no"];
  halted -> advance [label="yes"];

  advance -> nop;
  nop -> nop_handle [label="yes"];
  nop -> condtest [label="no"];
  nop_handle -> next;

  condtest -> skip [label="fails"];
  condtest -> decode [label="passes / n/a"];
  skip -> next;
  decode -> next;

  next -> start [label="next cycle", style=dashed, constraint=false];
}
\enddot

`CPU::step()` checks for a pending, unmasked interrupt before anything else; interrupt stacking is not allowed
(`is_interrupt()` returns false while the `in_interrupt` flag is already set), and handling one just redirects
`$pc` to the handler address rather than branching out of the cycle, so the handler's first instruction is
fetched and executed in the same `step()` call. `fetch()` bounds-checks `$pc` via `check_memory` before
touching DRAM, raising a `segfault` error and halting on failure, which is why `step()` re-checks
`is_running()` immediately after fetching. `_nop` is special-cased ahead of the conditional-test check (it can
optionally halt the CPU if `--halt-on-nop` was passed, for catching runaway execution into unwritten memory).
The conditional-test evaluation and the opcode `switch` both live in `execute()`, dispatching to one `exec_*`
handler per opcode with no function-pointer table. `exec_add`/`exec_sub`/`exec_mul`/`exec_div`/`exec_mod` all
expand from the same `ARITH_OPERATION` macro, so the six-datatype dispatch is written once and shared across
all five arithmetic opcodes rather than duplicated.

## The Visualiser

The visualiser is an FTXUI-based terminal UI that embeds the processor's source directly (its CMake target
compiles the processor's `.cpp` files straight into its own binary rather than shelling out to the `processor`
executable), so it can single-step execution and inspect live CPU/DRAM state in-process.

It is explicitly a four-artifact tool: given a base filename, it expects (or accepts explicitly via
`--edel`/`--asm`/`--reconstruction`/`--bin`) the `.edel` source, the compiler's `.asm` output, the assembler's
`-r` reconstructed-assembly listing, and the final binary, all four representations of the same program. It
traces each executed `$pc` back through the reconstructed assembly line, to the corresponding `.asm` source
line, to the originating `.edel` line, using the same shared generic `Graph` type the compiler uses for its
type-coercion lattice, and drives step-by-step execution through tabs for source view, register state, memory
contents, and execution control.
