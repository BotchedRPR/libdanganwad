# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name and language
project(wadproject LANGUAGES C)

# compiler flags
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# danganwad library
add_library(danganwad src/libdanganwad/wad.c)

# include dir
target_include_directories(danganwad PUBLIC include)

# main code
add_executable(wad_tool src/wad_tool/main.c)
target_link_libraries(wad_tool PRIVATE danganwad)

# testing binaries
add_executable(wad_test_extract tests/test_extract.c)
target_link_libraries(wad_test_extract PRIVATE danganwad)

# enable testing functionality
enable_testing()

# define tests
add_test(
  NAME extract_single_file
  COMMAND $<TARGET_FILE:wad_test_extract> 0
  )

add_test(
  NAME extract_single_file_from_dir
  COMMAND $<TARGET_FILE:wad_test_extract> 1
  )

add_test(
  NAME extract_multi_files
  COMMAND $<TARGET_FILE:wad_test_extract> 2
  )

add_test(
  NAME extract_multi_files_from_dir
  COMMAND $<TARGET_FILE:wad_test_extract> 2
  )
