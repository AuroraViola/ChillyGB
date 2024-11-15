cmake_minimum_required(VERSION 3.18)
project(ChillyGB C)

include(CTest)
enable_testing()

set(CMAKE_C_STANDARD 11)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        include(FetchContent)
        FetchContent_Declare(
                raylib_win64
                URL      https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_mingw-w64.zip
        )
        FetchContent_Declare(
                openpnp_capture_win64
                URL      https://github.com/openpnp/openpnp-capture/releases/download/v0.0.28/libopenpnp-capture-windows-latest-x86_64.dll
                DOWNLOAD_NO_EXTRACT TRUE
                DOWNLOAD_NAME openpnp-capture.dll
        )
        FetchContent_MakeAvailable(raylib_win64 openpnp_capture_win64)
        include_directories(${raylib_win64_SOURCE_DIR}/include)
        link_directories(${raylib_win64_SOURCE_DIR}/lib ${openpnp_capture_win64_SOURCE_DIR})
endif()
 
if (${PLATFORM} MATCHES "NX")
        include(FetchContent)
        FetchContent_Declare(
                        raylib_nx
                        GIT_REPOSITORY https://github.com/luizpestana/raylib-nx
                        GIT_TAG 5f62632744b82eaadd9eccd07492c456daabe52d
        )
        FetchContent_MakeAvailable(raylib_nx)
        add_custom_target(libraylib_nx COMMAND make PLATFORM=PLATFORM_NX ${raylib_nx_SOURCE_DIR}/src)
        include_directories(${raylib_nx_SOURCE_DIR}/src)
        link_directories(${raylib_nx_SOURCE_DIR}/src)
endif()

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
        src/includes/savestates.h
        src/modules/savestates.c
        src/modules/raylib-nuklear.c
        src/includes/serial.h
        src/modules/serial.c
        src/includes/camera.h
        src/modules/camera.c
        third-party/cJSON/cJSON.c
        src/includes/memory.h
        src/modules/memory.c
        src/includes/cheats.h
        src/modules/cheats.c
        src/modules/ui.c
        src/includes/ui.h
)
include_directories(
        third-party/openpnp-capture/include
        third-party/cJSON
        third-party/raylib-nuklear/include
)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_subdirectory(third-party/openpnp-capture)
    target_link_libraries(ChillyGB raylib m openpnp-capture)
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(ChillyGB raylib m openpnp-capture)
endif()
option(CUSTOM_OPEN_DIALOG "Enable custom open dialog box built with Nuklear" OFF)

if(CUSTOM_OPEN_DIALOG)
        target_compile_definitions(ChillyGB PRIVATE -DCUSTOM_OPEN_DIALOG)
endif()

if (${PLATFORM} MATCHES "NX")
  target_link_libraries(ChillyGB EGL GLESv2 glapi drm_nouveau nx stdc++)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.ncap
    COMMAND nacptool --create "ChillyGB" "AuroraViola" "v0.3.0" ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.nacp
    DEPENDS ChillyGB
  )
  add_custom_target(ChillyGB_ncap ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.ncap)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.nro
    COMMAND elf2nro ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.elf ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.nro --icon=${CMAKE_SOURCE_DIR}/res/icons/ChillyGB-512.png --nacp=${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.nacp --romfsdir=${CMAKE_SOURCE_DIR}/res/
    DEPENDS ChillyGB ChillyGB_ncap
  )
  add_custom_target(ChillyGB_nro ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/ChillyGB.nro)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_link_libraries(ChillyGB gdi32 winmm comdlg32)
endif()

add_custom_command(
        TARGET ChillyGB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res/
        ${CMAKE_CURRENT_BINARY_DIR}/res/
)

add_test(blargg/cpu_instrs
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/cpu_instrs/cpu_instrs.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/cpu_instrs/cpu_instrs-dmg-cgb.png"
        "--ticks" "300000000")

add_test(blargg/instr_timing
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/instr_timing/instr_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/instr_timing/instr_timing-dmg-cgb.png"
        "--ticks" "20000000")

add_test(blargg/interrupt_time
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/interrupt_time/interrupt_time.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/interrupt_time/interrupt_time-dmg.png"
        "--ticks" "20000000")

add_test(blargg/mem_timing
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/mem_timing/mem_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/mem_timing/mem_timing-dmg-cgb.png"
        "--ticks" "20000000")

add_test(blargg/mem_timing-2
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/mem_timing-2/mem_timing.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/mem_timing-2/mem_timing-dmg-cgb.png"
        "--ticks" "20000000")

## Tests not yet implemented

#add_test(blargg/dmg_sound
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/dmg_sound/dmg_sound.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/dmg_sound/dmg_sound-dmg.png"
#        "--ticks" "300000000")

#add_test(blargg/oam_bug
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/oam_bug/oam_bug.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/oam_bug/oam_bug-dmg.png"
#        "--ticks" "300000000")

#add_test(blargg/halt_bug
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/halt_bug.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/blargg/halt_bug-dmg-cgb.png"
#        "--ticks" "20000000")

#add_test(bully
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
#                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/bully/bully.gb"
#        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/bully/bully.png"
#        "--ticks" "20000000")

add_test(dmg-acid2
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/dmg-acid2/dmg-acid2.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/dmg-acid2/dmg-acid2-dmg.png"
        "--ticks" "4000000")

add_test(mooneye/manual-only/sprite_priority
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/mooneye-test-suite/manual-only/sprite_priority.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/mooneye-test-suite/manual-only/sprite_priority-dmg.png"
        "--ticks" "1000000")

add_test(mbc3-tester
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/mbc3-tester/mbc3-tester.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/mbc3-tester/mbc3-tester-dmg.png"
        "--ticks" "4000000")

add_test(scribbltest/lycscx
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/lycscx/lycscx.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/lycscx/lycscx-cgb-dmg.png"
        "--ticks" "1000000")

add_test(scribbltest/lycscy
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/lycscy/lycscy.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/lycscy/lycscy-cgb-dmg.png"
        "--ticks" "1000000")

add_test(scribbltest/palettely
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/palettely/palettely.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/palettely/palettely-dmg.png"
        "--ticks" "1000000")

add_test(scribbltest/scxly
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/scxly/scxly.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/scribbltests/scxly/scxly-dmg.png"
        "--ticks" "1000000")

add_test(turtle-tests/window_y_trigger
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/turtle-tests/window_y_trigger/window_y_trigger.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/turtle-tests/window_y_trigger/window_y_trigger.png"
        "--ticks" "1000000")

add_test(turtle-tests/window_y_trigger_wx_offscreen
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
                  "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/turtle-tests/window_y_trigger_wx_offscreen/window_y_trigger_wx_offscreen.gb"
        "--image" "${CMAKE_SOURCE_DIR}/third-party/game-boy-test-roms/artifacts/turtle-tests/window_y_trigger_wx_offscreen/window_y_trigger_wx_offscreen.png"
        "--ticks" "1000000")
