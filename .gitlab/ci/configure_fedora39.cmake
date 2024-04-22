# Fedora 34 ships with Java 11. Setting this as our version avoids warnings
# about not specifying a "bootstrap classpath".
set(VTK_JAVA_SOURCE_VERSION 11 CACHE STRING "")
set(VTK_JAVA_TARGET_VERSION 11 CACHE STRING "")

# Enable SPDX generation
set(VTK_GENERATE_SPDX ON CACHE BOOL "")

# Add rpath entries for dependencies.
set(CMAKE_INSTALL_RPATH "/usr/local/lib64" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
