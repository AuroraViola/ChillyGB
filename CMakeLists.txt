cmake_minimum_required(VERSION 3.28)
project(ChillyGB C)

include(CTest)
enable_testing()

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

add_test(blargg/cpu_instrs,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/cpu_instrs/cpu_instrs.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/cpu_instrs/cpu_instrs-dmg-cgb.png"
        "--ticks" "300000000")

add_test(blargg/instr_timing,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/instr_timing/instr_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/instr_timing/instr_timing-dmg-cgb.png"
        "--ticks" "20000000")

add_test(blargg/interrupt_time,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/interrupt_time/interrupt_time.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/interrupt_time/interrupt_time-dmg.png"
        "--ticks" "20000000")

add_test(blargg/mem_timing,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/mem_timing/mem_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/mem_timing/mem_timing-dmg-cgb.png"
        "--ticks" "20000000")

add_test(blargg/mem_timing-2,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/mem_timing-2/mem_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/mem_timing-2/mem_timing-dmg-cgb.png"
        "--ticks" "20000000")

## Tests not yet implemented

#add_test(blargg/dmg_sound,
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/dmg_sound/dmg_sound.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/dmg_sound/dmg_sound-dmg.png"
#        "--ticks" "300000000")

#add_test(blargg/oam_bug,
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/oam_bug/oam_bug.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/oam_bug/oam_bug-dmg.png"
#        "--ticks" "300000000")

#add_test(blargg/halt_bug,
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/halt_bug.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/blargg/halt_bug-dmg-cgb.png"
#        "--ticks" "20000000")

add_test(dmg-acid2,
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/dmg-acid2/dmg-acid2.gb"
        "--image" "${CMAKE_SOURCE_DIR}/game-boy-test-roms/artifacts/dmg-acid2/dmg-acid2-dmg.png"
        "--ticks" "4000000")
