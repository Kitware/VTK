cmake_minimum_required(VERSION 3.8)

include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# Pick up from where the configure left off.
ctest_start(APPEND)

include(ProcessorCount)
ProcessorCount(nproc)

set(CTEST_MEMORYCHECK_TYPE "$ENV{CTEST_MEMORYCHECK_TYPE}")
set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "$ENV{CTEST_MEMORYCHECK_SANITIZER_OPTIONS}")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "lsan")
  set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE "${CTEST_SOURCE_DIRECTORY}/Utilities/DynamicAnalysis/lsan.supp")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "tsan")
  set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE "${CTEST_SOURCE_DIRECTORY}/Utilities/DynamicAnalysis/tsan.supp")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/ctest_exclusions.cmake")
ctest_memcheck(
  PARALLEL_LEVEL "${nproc}"
  RETURN_VALUE test_result
  EXCLUDE "${test_exclusions}"
  OUTPUT_JUNIT "${CTEST_BINARY_DIRECTORY}/junit.xml"
  DEFECT_COUNT defects)
ctest_submit(PARTS Test)
ctest_submit(PARTS Memcheck)

if (test_result)
  message(FATAL_ERROR
    "Failed to test")
endif ()

if (defects)
  message(FATAL_ERROR
    "Found ${defects} memcheck defects")
endif ()
