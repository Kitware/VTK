# Install to the DESTDIR directly.
set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "")

# Simplify the install tree for upload.
set(VTK_VERSIONED_INSTALL OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
