set(CMAKE_INSTALL_RPATH "/usr/local/lib64" CACHE STRING "")

set(VTK_ENABLE_OSPRAY ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora44.cmake")
