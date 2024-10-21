# raylib_nuklear
add_library(raylib_nuklear INTERFACE)
target_include_directories(raylib_nuklear INTERFACE .)
install(FILES
        raylib-nuklear.h
        nuklear.h
    DESTINATION include
)
