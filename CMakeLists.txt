cmake_minimum_required(VERSION 3.22)
project(platformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: ccache for faster rebuilds
find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

# godot-cpp
add_subdirectory(godot-cpp)

# Your source files
add_library(platformer SHARED
    src/register_types.cpp
    src/player.cpp
    src/level.cpp  
    src/win_platform.cpp
    src/win_screen.cpp
    src/main_menu.cpp
    src/frame.cpp
    src/platform.cpp
    src/collectable.cpp
    src/orb.cpp
    src/spike.cpp
    src/lose_screen.cpp
    src/kill_platform.cpp
)

target_include_directories(platformer PRIVATE src/)
target_link_libraries(platformer PRIVATE godot-cpp)

# Output directly into bin/
set_target_properties(platformer PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "platformer"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)