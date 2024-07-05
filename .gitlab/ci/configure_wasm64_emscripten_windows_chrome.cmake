include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")

set(VTK_WEBASSEMBLY_64_BIT ON CACHE BOOL "")

get_filename_component(project_dir "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
set(chrome_executable "${project_dir}/.gitlab/chrome/chrome.exe")
set(chrome_arguments "--disable-restore-session-state --no-default-browser-check --no-first-run --incognito --disable-application-cache --new-tab --js-flags=--experimental-wasm-memory64")
set(VTK_TESTING_WASM_ENGINE "${chrome_executable}" CACHE STRING "")
set(VTK_TESTING_WASM_ENGINE_ARGUMENTS "${chrome_arguments}" CACHE STRING "")

set(VTK_ENABLE_WRAPPING OFF CACHE BOOL "") # fails to create wrapping files, even with NODERAWFS
set(VTK_WRAP_SERIALIZATION OFF CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_libproj NO CACHE STRING "") # fails to create proj.db, even with NODERAWFS
