set(VTK_ENABLE_SANITIZER ON CACHE BOOL "")
set(VTK_SANITIZER "undefined" CACHE STRING "")

# Enable VTK_USE_FUTURE_CONST so that at something on CI tests it.
set(VTK_USE_FUTURE_CONST ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
