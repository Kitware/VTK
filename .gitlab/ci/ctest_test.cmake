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

set(maybe_include_label_mangling "")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "mangling")
  list(APPEND maybe_include_label_mangling INCLUDE_LABEL MANGLING)
endif()

ctest_test(APPEND
  PARALLEL_LEVEL "${nproc}"
  TEST_LOAD "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  ${maybe_include_label_mangling}
  OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml"
  REPEAT UNTIL_PASS:3)
ctest_submit(PARTS Test)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_annotation.cmake")
if (DEFINED build_id)
  ctest_annotation_report("${CTEST_BINARY_DIRECTORY}/annotations.json"
    "Build Summary" "https://open.cdash.org/build/${build_id}"
    "All Tests"     "https://open.cdash.org/viewTest.php?buildid=${build_id}"
    "Test Failures" "https://open.cdash.org/viewTest.php?onlyfailed&buildid=${build_id}"
    "Tests Not Run" "https://open.cdash.org/viewTest.php?onlynotrun&buildid=${build_id}"
    "Test Passes"   "https://open.cdash.org/viewTest.php?onlypassed&buildid=${build_id}"
  )
endif ()

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()
