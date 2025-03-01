#!/bin/bash
# this script compiles `program.edel'
out/compiler program.edel -o out/program.asm -d
out/assembler out/program.asm -o out/program -d -r out/program.s -l assembler/lib
