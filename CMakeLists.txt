cmake_minimum_required(VERSION 3.28)
project(ChillyGB C)

set(CMAKE_C_STANDARD 11)

add_executable(ChillyGB main.c
        cpu.c
        cpu.h
        opcodes.c
        opcodes.h
        ppu.h
        ppu.c
        input.c
        input.h
        apu.h
        apu.c
        timer.h
        timer.c
        cartridge.h
        cartridge.c
)
target_link_libraries(ChillyGB raylib m)
