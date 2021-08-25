include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Create a new entry if needed,
# else pick up from where the configure left off.
if("$ENV{CTEST_NEW_CDASH_SUBMISSION}" STREQUAL "True")
  ctest_start(Experimental TRACK "${ctest_track}")

  # Gather update information.
  find_package(Git)
  set(CTEST_UPDATE_VERSION_ONLY ON)
  set(CTEST_UPDATE_COMMAND "${GIT_EXECUTABLE}")
  ctest_update()
  ctest_submit(PARTS Update)
else()
  ctest_start(APPEND)
endif()

include(ProcessorCount)
ProcessorCount(nproc)
if (NOT "$ENV{CTEST_MAX_PARALLELISM}" STREQUAL "")
  if (nproc GREATER "$ENV{CTEST_MAX_PARALLELISM}")
    set(nproc "$ENV{CTEST_MAX_PARALLELISM}")
  endif ()
endif ()

# Default to a reasonable test timeout.
set(CTEST_TEST_TIMEOUT 100)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_exclusions.cmake")
ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  TEST_LOAD "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
