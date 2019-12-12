# Detect if we've run with these flags before or not.
string(MD5 flags_hash "${CMAKE_SHARED_LINKER_FLAGS}")
if (NOT undefined_symbol_flag_hash STREQUAL flags_hash)
  # If we have a try_compile result, but no validation hash,
  # `-Dvtk_undefined_symbols_allowed` was passed on the command line; trust it.
  # Further changes to CMAKE_SHARED_LINKER_FLAGS will cause a recheck however.
  if (DEFINED undefined_symbol_flag_hash)
    # New (or untested) flags; clear the old result.
    unset(vtk_undefined_symbols_allowed CACHE)
  endif ()
  # Store that we've tested this hash.
  set(undefined_symbol_flag_hash "${flags_hash}"
    CACHE INTERNAL "undefined symbol detection hash")
endif ()

# If we don't have a result, run its test.
if (NOT DEFINED "vtk_undefined_symbols_allowed")
  set(test_project_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/vtk_undefined_symbols_allowed")
  file(WRITE "${test_project_dir}/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.3...3.12)
project(undefined C)
add_library(undefined SHARED uses_undefined.c)
add_executable(undefined_exe main.c)
target_link_libraries(undefined_exe PRIVATE undefined)
")
  file(WRITE "${test_project_dir}/uses_undefined.c"
"extern int undefined(void);
int uses_undefined(void) {
  return undefined() + 1;
}
")
  file(WRITE "${test_project_dir}/main.c"
"extern int uses_undefined(void);
int main(void) {
  return uses_undefined();
}
")

  try_compile(vtk_undefined_symbols_allowed
    "${test_project_dir}"
    "${test_project_dir}"
    undefined
    CMAKE_FLAGS
      "-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}"
      ${_rpath_arg}
    OUTPUT_VARIABLE output)

  if (vtk_undefined_symbols_allowed)
    message(STATUS "Performing Test vtk_undefined_symbols_allowed - Success")
  else ()
    message(STATUS "Performing Test vtk_undefined_symbols_allowed - Failed")
    file(APPEND "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log"
      "Performing Test vtk_undefined_symbols_allowed failed with the "
      "following output:\n${output}\n")
  endif ()
endif ()
