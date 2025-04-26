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
- `out/` contains the compiled documentation PDFs, as well as where the built binaries are placed.
- `shared/` contains source code files used by multiple components.

## Building the Project

This project uses CMake to help build each component.
The top-level `CMakeLists.txt` defines four processes for building the four project components.

Requirements:
- CMake `>= 3.6`.
- A C++ compiler supporting `C++20`.
- Build tool such as `make` or `ninja` (see below).

Follow these steps to build:
1. Open a terminal instance in the top-level directory.
2. Run `cmake .`.
This will download necessary dependencies and generate build scripts.
3. Run the generated build scripts.
This varies between platforms:
- For Linux (WSL Ubuntu), this requires running `make`.
- For Windows, this requires running `ninja`.

At this stage, hints should be provided in the CMake output: 
for example, "Built for ninja" or similar.
4. All four project components should be built.
These will be located in the `out/` directory.

### Development System

While this project is multi-platform, there may be issues with building, such as if your C++ compiler does not fully support C++20.
For completeness, below lists the system which was used to develop this project:
- OS: `WSL2 Ubuntu 22.04.2 LTS`.
- CMake: `WSL CMake version 3.22.1`.
- Make: `GNU Make 4.3`.
- Compiler: `g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0`.
- Debugger: `WSL GDB version 12.1`.

The codebase was developed using IntelliJ's CLion IDE.
The build and execution scripts used can be found under `.run/`.

### Dependencies

All dependencies are handled by CMake, both in terms of downloading and building.
Other than support for C++20, the project has the following dependencies:
- `FTXUI v5.1.0`.
Used by the visualiser, this lightweight library offers support for constructing Textual User Interfaces (TUIs).
GitHub: https://github.com/ArthurSonzogni/FTXUI.

## Running the Project

Two script files have been written to aid with using the application:
1. Write Edel code in `program.edel`.
2. Run `compile.sh` to compile this program (`out/program.asm`) and assemble this into machine code (`out/program`).
3. Run `visualise.sh` to run the visualiser on the Edel program.

For more information, see provided documentation, especially `out/visualiser.pdf` which has a section explaining how to run the visualiser application.
