# Disable `.pyi` files. Libraries don't load properly.
set(VTK_BUILD_PYI_FILES OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_wheel.cmake")
