# testing binaries
add_executable(wad_test_extract tests/test_extract.c)
add_executable(wad_test_repack  tests/test_repack.c)

target_link_libraries(wad_test_extract PRIVATE danganwad)
target_link_libraries(wad_test_repack  PRIVATE danganwad)

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
  COMMAND $<TARGET_FILE:wad_test_extract> 3
  )

add_test(
  NAME pack_single_file
  COMMAND $<TARGET_FILE:wad_test_repack> 0
  )

add_test(
  NAME pack_multi_files
  COMMAND $<TARGET_FILE:wad_test_repack> 1
  )

add_test(
  NAME pack_multi_files_in_dirs
  COMMAND $<TARGET_FILE:wad_test_repack> 2
  )
