cmake_minimum_required(VERSION 3.16)
project(RISC-V)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall")

add_executable(code
        src/main.cpp
        src/Memory.hpp
        src/Instruction.hpp
        src/Stage.hpp
        src/Buffer.hpp
        src/Register.hpp
        src/CPU.hpp
        src/Predictor.hpp
        src/LocalTest.h
)