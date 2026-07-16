#!/bin/bash
# This script build a Doxygen documentation website from Doxygen comments in the code
doxygen Doxyfile 2>&1 | tee doxygen-warnings.log
