cmake_minimum_required(VERSION 3.28)
project(ChillyGB C)

set(CMAKE_C_STANDARD 11)

add_executable(ChillyGB src/main.c
        src/modules/cpu.c
        src/includes/cpu.h
        src/modules/opcodes.c
        src/includes/opcodes.h
        src/includes/ppu.h
        src/modules/ppu.c
        src/modules/input.c
        src/includes/input.h
        src/includes/apu.h
        src/modules/apu.c
        src/includes/timer.h
        src/modules/timer.c
        src/includes/cartridge.h
        src/modules/cartridge.c
        src/includes/debug.h
        src/modules/debug.c
        src/includes/settings.h
        src/modules/settings.c
        src/includes/open_dialog.h
        src/modules/open_dialog.c
)
target_link_libraries(ChillyGB raylib m)

add_custom_command(
        TARGET ChillyGB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res/
        ${CMAKE_CURRENT_BINARY_DIR}/res/
)
