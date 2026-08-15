cmake_minimum_required(VERSION 3.16)
project(bfxr C)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform detection
if(WIN32 OR MINGW OR MSYS)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
else()
    set(PLATFORM_LINUX TRUE)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(COMMON_SRCS
    os_util.c
    bfxr_params.c
    bfxr_generator.c
    bfxr_wav.c
    bfxr_file.c
    bfxr_fft.c
    bfxr_config.c
    ini.c
)

add_executable(bfxr main.c ${COMMON_SRCS})

set(GUI_SRCS
    gui.c
    bfxr_uicomp.c
    bfxr_ui.c
    bfxr_vis.c
    ${COMMON_SRCS}
)

add_executable(bfxr_gui ${GUI_SRCS})

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_MakeAvailable(raylib)

target_link_libraries(bfxr_gui PRIVATE raylib)

if(PLATFORM_WINDOWS)
    target_link_libraries(bfxr_gui PRIVATE gdi32 winmm opengl32 pthread)
elseif(PLATFORM_MACOS)
    target_link_libraries(bfxr_gui PRIVATE pthread m)
else()
    target_link_libraries(bfxr_gui PRIVATE m pthread dl rt X11)
    target_link_libraries(bfxr PRIVATE m)
endif()

# Stage the GUI font next to the built binary so it is self-contained
add_custom_command(TARGET bfxr_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/Cadman_Bold.otf"
    $<TARGET_FILE_DIR:bfxr_gui>
)
