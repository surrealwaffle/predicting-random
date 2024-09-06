cmake_minimum_required(VERSION 3.28)
project(predicting-random
    DESCRIPTION "Solver for glibc random()"
    HOMEPAGE_URL https://github.com/surrealwaffle/predicting-random
    LANGUAGES CXX
)

# Provides two targets:
# predicting-random-solver, the library portion of the project which provides the 
# solver.
#
# predicting-random-tester, an executable which, given a seed, runs the solver on 
# output provided by a generated that conforms to glibc random().

add_library(predicting-random-solver INTERFACE)
target_include_directories(predicting-random-solver
    INTERFACE
        include
)
target_compile_features(predicting-random-solver
    INTERFACE
        cxx_std_20
)
set_target_properties(predicting-random-solver
    PROPERTIES
        CXX_STANDARD_REQUIRED ON
)

add_executable(predicting-random-tester)
target_sources(predicting-random-tester
    PRIVATE
        test_solver.cpp
)
target_link_libraries(predicting-random-tester
    PRIVATE
        predicting-random-solver
)