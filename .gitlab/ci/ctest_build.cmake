include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Pick up from where the configure left off.
ctest_start(APPEND)

set(targets_to_build "all")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "doxygen")
  list(APPEND targets_to_build
    DoxygenDoc)
endif ()

foreach (target IN LISTS targets_to_build)
  set(build_args)
  if (NOT target STREQUAL "all")
    list(APPEND build_args TARGET ${target})
  endif ()

  if (CTEST_CMAKE_GENERATOR MATCHES "Make")
    # Drop the `-i` flag without remorse.
    set(CTEST_BUILD_COMMAND "make -j 4 ${CTEST_BUILD_FLAGS}")

    if (NOT target STREQUAL "all")
      set(CTEST_BUILD_COMMAND "${CTEST_BUILD_COMMAND} ${target}")
    endif ()
  endif ()

  ctest_build(
    RETURN_VALUE    build_result
    ${build_args})

  ctest_submit(PARTS Build)
endforeach ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "doxygen")
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
            -DCMAKE_INSTALL_COMPONENT=doxygen
            -P "${CTEST_BINARY_DIRECTORY}/cmake_install.cmake"
    RESULT_VARIABLE doxygen_result
    OUTPUT_FILE "${CTEST_SOURCE_DIRECTORY}/doxygen_output.log"
    ERROR_FILE  "${CTEST_SOURCE_DIRECTORY}/doxygen_output.log")

  if (doxygen_result)
    message("Doxygen failed to install")
    set(build_result "${doxygen_result}")
  endif ()
endif ()

file(GLOB logs
  "${CTEST_SOURCE_DIRECTORY}/compile_output.log"
  "${CTEST_SOURCE_DIRECTORY}/doxygen_output.log"
  "${CTEST_SOURCE_DIRECTORY}/prepare_output.log")
if (logs)
  list(APPEND CTEST_NOTES_FILES ${logs})
  ctest_submit(PARTS Notes)
endif ()

if (build_result)
  message(FATAL_ERROR
    "Failed to build")
endif ()
