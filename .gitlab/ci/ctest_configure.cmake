include("${CMAKE_CURRENT_LIST_DIR}/gitlab_ci.cmake")

set(cmake_args
  -G "${CTEST_CMAKE_GENERATOR}"
  "-DCMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION}"
  -C "${CMAKE_CURRENT_LIST_DIR}/configure_$ENV{CMAKE_CONFIGURATION}.cmake")

list(INSERT _ctest_configure_command 1 ${cmake_args})
string(REPLACE ";" " " CTEST_CONFIGURE_COMMAND "${_ctest_configure_command}")

# Create an entry in CDash.
ctest_start(Experimental TRACK "${ctest_track}")

# Gather update information.
find_package(Git)
set(CTEST_UPDATE_VERSION_ONLY ON)
set(CTEST_UPDATE_COMMAND "${GIT_EXECUTABLE}")
ctest_update()

# Configure the project.
ctest_configure(
  RETURN_VALUE configure_result)

# Read the files from the build directory.
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

# We can now submit because we've configured. This is a cmb-superbuild-ism.
ctest_submit(PARTS Update)
ctest_submit(
  PARTS Configure
  BUILD_ID build_id)

include("${CMAKE_CURRENT_LIST_DIR}/ctest_annotation.cmake")
if (DEFINED build_id)
  ctest_annotation_report("${CTEST_BINARY_DIRECTORY}/annotations.json"
    "Build Summary" "https://open.cdash.org/build/${build_id}"
    "Update" "https://open.cdash.org/build/${build_id}/update"
    "Configure" "https://open.cdash.org/build/${build_id}/configure"
  )
  store_build_id("${build_id}")
endif ()

if (configure_result)
  message(FATAL_ERROR
    "Failed to configure")
endif ()
