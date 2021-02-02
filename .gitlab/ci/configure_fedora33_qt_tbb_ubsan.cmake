set(VTK_ENABLE_SANITIZER ON CACHE BOOL "")
set(VTK_SANITIZER "address" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
