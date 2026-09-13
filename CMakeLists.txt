# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name and language
project(wadproject LANGUAGES C)

# compiler flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -DSRCDIR="${CMAKE_SOURCE_DIR}/"
)

# danganwad library
add_library(danganwad src/libdanganwad/wad.c)

# include dir
target_include_directories(danganwad PUBLIC include)

# main code
add_executable(wad_tool src/wad_tool/main.c)
target_link_libraries(wad_tool PRIVATE danganwad)

include(tests/CMakeLists.txt)
