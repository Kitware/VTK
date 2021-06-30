# Install to the DESTDIR directly.
set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "")

# Simplify the install tree for upload.
set(VTK_VERSIONED_INSTALL OFF CACHE BOOL "")

# Disable testing since Doxygen doesn't actually test.
set(VTK_BUILD_TESTING OFF CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
