cmake_minimum_required(VERSION 3.16)
project(RISC-V)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall")

add_executable(code
        main.cpp
        Memory.hpp
        Instruction.hpp
        Stage.hpp
        Buffer.hpp
        Register.hpp
        CPU.hpp
        Predictor.hpp
        LocalTest.h
)
