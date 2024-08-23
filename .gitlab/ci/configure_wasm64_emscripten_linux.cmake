# Disable testing for non-threaded wasm builds because if testing is enabled,
# VTK cmake enables threads.
set(VTK_BUILD_TESTING OFF CACHE STRING "")
# Ensure threads are not enabled.
set(VTK_WEBASSEMBLY_THREADS OFF CACHE BOOL "")
set(VTK_WEBASSEMBLY_64_BIT ON CACHE BOOL "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")
