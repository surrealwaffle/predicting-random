# predicting-random
Solver for glibc type 3 `random()`

# Building
Building has to be done manually for this project. It's small enough for it anyway.

`g++ -std=c++20 test_solver.cpp` builds an executable that attempts to solve the 
PRNG of the seed supplied on the command line.

For `g++`, it is recommended to use `-O3 -march=native` for performance.
Using `clang++`, it is recommended to use `-O2 -march=native`.

# How It Works
The structure of `glibc` LFSR-based `random()` allows one to observe output from 
`random()` and build a system of linear equations which can be used to solve for 
the state of the PRNG. For more details, see the preamble of the file `solver.hpp`.