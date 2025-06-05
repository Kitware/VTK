# Ensure threads are not enabled.
set(VTK_WEBASSEMBLY_THREADS OFF CACHE BOOL "")
set(VTK_WEBASSEMBLY_64_BIT ON CACHE BOOL "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")
