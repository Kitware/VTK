# Enable SPDX generation
set(VTK_GENERATE_SPDX ON CACHE BOOL "")

# Add rpath entries for dependencies.
set(CMAKE_INSTALL_RPATH "/usr/local/lib64:$ENV{CI_PROJECT_DIR}/.gitlab/qt6/lib" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
