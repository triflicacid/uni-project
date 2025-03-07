#!/bin/bash
# this script executes the visualiser based on the output of `compile.sh'
#out/visualiser --edel program.edel out/program 2> cerr.log
gdb --args out/visualiser "--edel" program.edel out/program
