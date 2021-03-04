include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

set(CTEST_SOURCE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/Testing/External")
set(cmake_args
  "-DCMAKE_BUILD_TYPE:STRING=Release"
  "-DCMAKE_PREFIX_PATH:PATH=${CTEST_BINARY_DIRECTORY}/install"
  "-DVTK_USE_LARGE_DATA:BOOL=ON")

# Create an entry in CDash.
ctest_start(Experimental TRACK "${ctest_track}")

# Gather update information.
find_package(Git)
set(CTEST_UPDATE_VERSION_ONLY ON)
set(CTEST_UPDATE_COMMAND "${GIT_EXECUTABLE}")
ctest_update()

# Configure the project.
ctest_configure(
  OPTIONS "${cmake_args}"
  RETURN_VALUE configure_result)

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# We can now submit because we've configured. This is a cmb-superbuild-ism.
ctest_submit(PARTS Update)
ctest_submit(PARTS Configure)

if (configure_result)
  message(FATAL_ERROR
    "Failed to configure")
endif ()

include(ProcessorCount)
ProcessorCount(nproc)

if (CTEST_CMAKE_GENERATOR MATCHES "Make")
  # Drop the `-i` flag without remorse.
  set(CTEST_BUILD_COMMAND "make -j ${nproc} ${CTEST_BUILD_FLAGS}")
endif ()

ctest_build(
  NUMBER_WARNINGS num_warnings
  RETURN_VALUE    build_result
  ${build_args})

ctest_submit(PARTS Build)

if (build_result)
  file(GLOB logs
    "${CTEST_SOURCE_DIRECTORY}/compile_output.log")
  if (logs)
    list(APPEND CTEST_NOTES_FILES ${logs})
    ctest_submit(PARTS Notes)
  endif ()
endif ()

if (build_result)
  message(FATAL_ERROR
    "Failed to build")
endif ()

# Default to a reasonable test timeout.
set(CTEST_TEST_TIMEOUT 100)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_exclusions.cmake")
ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
