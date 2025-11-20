# Ensures that -pthreads is disabled for WebAssembly builds.
set(VTK_WEBASSEMBLY_THREADS OFF CACHE BOOL "")
# Prevent file lock issues during linking by limiting to 1 job at a time.
# Upstream issue: https://github.com/emscripten-core/emscripten/issues/24609
# Possible fix: https://github.com/emscripten-core/emscripten/pull/25771
# After the fix is released, we can set this to a higher value.
set(VTK_WEBASSEMBLY_JOB_POOL_LINK_SIZE 1 CACHE STRING "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")
