include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")

get_filename_component(project_dir "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
set(chrome_executable "${project_dir}/.gitlab/chrome/chrome")
set(chrome_arguments "--disable-restore-session-state --no-default-browser-check --no-sandbox --no-first-run --incognito --disable-application-cache --new-tab")
set(VTK_TESTING_WASM_ENGINE "${chrome_executable}" CACHE STRING "")
set(VTK_TESTING_WASM_ENGINE_ARGUMENTS "${chrome_arguments}" CACHE STRING "")
