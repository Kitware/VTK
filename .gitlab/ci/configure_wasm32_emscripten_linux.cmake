# Ensure threads are not enabled.
set(VTK_WEBASSEMBLY_THREADS OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")
