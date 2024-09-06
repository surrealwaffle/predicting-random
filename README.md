# predicting-random
Solver for glibc type 3 `random()`

# Building
This project can be built using CMake.

```
cmake -B build
cd build
cmake --build .
```

This project requires a C++20 compiler, mostly for concepts and some basic standard 
library features. 

This project provides two targets:
 * `predicting-random-solver`, the library portion of the project; and,
 * `predicting-random-tester`, which verifies the solver from the library portion of the target against a given seed.

Critical portions of the code can be auto-vectorized. If you are benchmarking the 
code, it is recommended to compile on `-O3` and `-O2` with `g++` and `clang++`, 
respectively.

# How It Works
The structure of `glibc` LFSR-based `random()` allows one to observe output from 
`random()` and build a system of linear equations which can be used to solve for 
the state of the PRNG. For more details, see the preamble of the file 
[solver.hpp](/include/solver.hpp).