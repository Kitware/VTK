include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")

# Disable testing for non-threaded wasm builds because threads are crucial for test data loading.
set(VTK_BUILD_TESTING OFF)
