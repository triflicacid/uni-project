# Uni Project

This repository holds the source code to my third year project.
The title of my project is `A code execution toolchain targeted at learners: from compilation to emulating a custom RISC processor`.

As a brief overview, this project is an educational tool for exploring the code execution toolchain.
It has three core components which make up the pipeline: a processor emulator, an assembler, and a compiler.
A fourth component, the visualiser, ties these three together in a visual and accessible way, allowing learners to explore the pipeline and observe *how* code is translated from source to machine code.

Documentation on all the components is available in the `out` directory.

## Project Structure

The project components are located in the following directories:
- `assembler/` for the assembler, assembly -> machine code.
- `compiler/` for the compiler, Edel -> assembly.
- `processor/` for the processor emulator, executes machine code.
- `visualiser/` for the visualiser application.

The other directories are as follows:
- `docs/` contains documentation written in LaTeX.
- `out/` contains the compiled documentation PDFs.
It is also where the built applications will reside, as well as the processor Edel file.
- `shared/` contains source code files used by multiple components.

## Building the Project

This project uses CMake to help build each component.
The top-level `CMakeLists.txt` defines four processes for building the four project components.

To build the project, `CMake >= 3.6` is required.
Each component is written in C++ using the C++20 standard, so a C++ compiler is also required.
The project was developed in IntelliJ's CLion, and the build/run configurations used are provided under the `.run/` directory.

Additionally, the visualiser component makes use of the third-party FTXUI library v5.1.0.
GitHub: https://github.com/ArthurSonzogni/FTXUI.

The project has been designed and written to be cross-platform.
The project has was developed using WSL2 Ubuntu and its native toolchain.
Debugging was done via WSL's GDB.

## Running the Project

Two script files have been written to aid with using the application:
1. Write Edel code in `program.edel`.
2. Run `compile.sh` to compile this program (`out/program.asm`) and assemble this into machine code (`out/program`).
3. Run `visualise.sh` to run the visualiser on the Edel program.

For more information, see provided documentation, especially `out/visualiser.pdf` which has a section explaining how to run the visualiser application.
