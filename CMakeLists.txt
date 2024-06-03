cmake_minimum_required(VERSION 3.28)
project(ChillyGB C)

set(CMAKE_C_STANDARD 11)

#find_library(RAYLIB raylib)
add_executable(ChillyGB main.c
        cpu.c
        cpu.h
        opcodes.c
        opcodes.h
        ppu.h
        ppu.c
        input.c
        input.h
)
target_link_libraries(ChillyGB raylib)