include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ctest_annotation.cmake")

set(annotation_report_file "${CTEST_BINARY_DIRECTORY}/annotations.json")
set(annotation_report)

set(CTEST_SOURCE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/Testing/External")
# The build directory holds VTK's build tree from the artifacts, so ignore its
# cache.
set(cmake_args
  "--fresh"
  "-DCMAKE_BUILD_TYPE:STRING=Release"
  "-DCMAKE_PREFIX_PATH:PATH=${CTEST_BINARY_DIRECTORY}/install"
  "-DVTK_USE_LARGE_DATA:BOOL=ON"
  "--no-warn-unused-cli")

# Use sccache when the job sets it up.
find_program(sccache_executable sccache)
if (sccache_executable)
  list(APPEND cmake_args
    "-DCMAKE_C_COMPILER_LAUNCHER:STRING=${sccache_executable}"
    "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=${sccache_executable}"
    "-DCMAKE_CUDA_COMPILER_LAUNCHER:STRING=${sccache_executable}")
endif ()

# This project does not use the `.gitlab/ci/configure_*.cmake` files, so any
# policy defaults needed by the dependencies found through `find_package(VTK)`
# have to be repeated here. Fedora has Boost's own CMake config files
# installed; without this, `find_package(Boost)` calls emit a CMP0167 dev
# warning which the warning check turns into a failure.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  list(APPEND cmake_args
    "-DCMAKE_POLICY_DEFAULT_CMP0167:STRING=NEW")
endif ()

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
ctest_submit(PARTS Configure
  BUILD_ID build_id)

if (DEFINED build_id)
  # The tests run in another job, which submits to this build.
  store_build_id("${build_id}")
  list(APPEND annotation_report
    "Build Summary" "https://open.cdash.org/build/${build_id}"
    "Update" "https://open.cdash.org/build/${build_id}/update"
    "Configure" "https://open.cdash.org/build/${build_id}/configure"
  )
endif ()

if (configure_result)
  ctest_annotation_report("${annotation_report_file}" ${annotation_report})
  ctest_submit(PARTS Done)
  message(FATAL_ERROR
    "Failed to configure")
endif ()

include(ProcessorCount)
ProcessorCount(nproc)
if (NOT "$ENV{CTEST_MAX_PARALLELISM}" STREQUAL "")
  if (nproc GREATER "$ENV{CTEST_MAX_PARALLELISM}")
    set(nproc "$ENV{CTEST_MAX_PARALLELISM}")
  endif ()
endif ()

if (CTEST_CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  set(CTEST_BUILD_FLAGS "-j${nproc} -l${nproc}")
elseif (CTEST_CMAKE_GENERATOR MATCHES "Ninja")
  set(CTEST_BUILD_FLAGS "-l${nproc}")
endif ()

if (CTEST_CMAKE_GENERATOR MATCHES "Make")
  # Drop the `-i` flag without remorse.
  set(CTEST_BUILD_COMMAND "make ${CTEST_BUILD_FLAGS}")
endif ()

ctest_build(
  NUMBER_ERRORS num_errors
  NUMBER_WARNINGS num_warnings
  RETURN_VALUE    build_result
  ${build_args})

ctest_submit(PARTS Build)

if (DEFINED build_id)
  list(APPEND annotation_report
      "Build Errors (${num_errors})" "https://open.cdash.org/viewBuildError.php?buildid=${build_id}"
      "Build Warnings (${num_warnings})" "https://open.cdash.org/viewBuildError.php?type=1&buildid=${build_id}"
  )
endif ()

if (build_result)
  file(GLOB logs
    "${CTEST_SOURCE_DIRECTORY}/compile_output.log")
  if (logs)
    list(APPEND CTEST_NOTES_FILES ${logs})
    ctest_submit(PARTS Notes)
  endif ()
endif ()

if (build_result)
  ctest_annotation_report("${annotation_report_file}" ${annotation_report})
  ctest_submit(PARTS Done)
  message(FATAL_ERROR
    "Failed to build")
endif ()

file(WRITE "${CTEST_SOURCE_DIRECTORY}/compile_num_warnings.log" "${num_warnings}")

# The tests run in another job, using `ctest_test.cmake`.
ctest_annotation_report("${annotation_report_file}" ${annotation_report})
