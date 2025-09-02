cmake_minimum_required(VERSION 3.29)

if (NOT DEFINED TESTING_WASM_ENGINE OR NOT TESTING_WASM_ENGINE)
  message(FATAL_ERROR "TESTING_WASM_ENGINE not specified!")
  cmake_language(EXIT 1)
endif()

if (NOT DEFINED TESTING_WASM_HTML_TEMPLATE OR NOT TESTING_WASM_HTML_TEMPLATE)
  message(FATAL_ERROR "TESTING_WASM_HTML_TEMPLATE not specified!")
  cmake_language(EXIT 1)
endif()

if (NOT DEFINED TEST_NAME OR NOT TEST_NAME)
  message(FATAL_ERROR "TEST_NAME not specified!")
  cmake_language(EXIT 1)
endif()

if (NOT DEFINED EXIT_AFTER_TEST)
  message(FATAL_ERROR "EXIT_AFTER_TEST not specified!")
  cmake_language(EXIT 1)
endif()

set(TEST_EXECUTABLE "")
set(TEST_ARGS "")

# Find position of argument after the '--' separator.
set(ARG_START 0)
math(EXPR ARG_MAX "${CMAKE_ARGC} - 1")
foreach(i RANGE ${ARG_MAX})
  if ("${CMAKE_ARGV${i}}" STREQUAL "--")
    math(EXPR ARG_START "${i} + 1")
    break()
  endif ()
endforeach()

# find test executable ".js" file and append all other arguments into TEST_ARGS
foreach(i RANGE ${ARG_START} ${ARG_MAX})
  if ("${CMAKE_ARGV${i}}" MATCHES "\.js$")
    set(TEST_EXECUTABLE "${CMAKE_ARGV${i}}")
  else ()
    list(APPEND TEST_ARGS "${CMAKE_ARGV${i}}")
  endif ()
endforeach()

if (NOT TEST_EXECUTABLE)
  message(STATUS "TEST_EXECUTABLE not specified. Skipping...")
  return()
endif()

function(generate_index_html js test_args exit_after_test url output_file)
  file(READ "${js}" javascript)
  set(close_window "")
  # right now we only use webgpu for the unit tests in the RenderingWebGPU module.
  set(graphics_backend "OPENGL") # default equivalent to WebGL2
  if (exit_after_test)
    set(close_window "window.close();")
  endif ()
  if ("${js}" MATCHES "WebGPU")
    set(graphics_backend "WEBGPU")
  endif ()
  configure_file("${TESTING_WASM_HTML_TEMPLATE}" "${output_file}" @ONLY)
endfunction()

function(extract_vtk_wasm_test_exit_code log_line exit_code)
  if (log_line MATCHES "vtk-wasm-test-exit-code=([-]?[0-9]+)")
    set(${exit_code} ${CMAKE_MATCH_1} PARENT_SCOPE)
  else()
    set(${exit_code} 1 PARENT_SCOPE)
  endif()
endfunction()

set(USER_PROFILE "unknown")
string(REPLACE ":" "" USER_PROFILE "${TEST_NAME}")
set(USER_PROFILE_DIR "${TEST_OUTPUT_DIR}/${USER_PROFILE}")

set(HTTP_SERVER_URL "http://localhost:8000")
if (EXISTS "${TEST_OUTPUT_DIR}/vtkhttp.lock")
  # The vtkhttp.lock has json. it is written by `server.js`.
  file(READ "${TEST_OUTPUT_DIR}/vtkhttp.lock" HTTP_LOCK)
  # pull address and port
  string(JSON HTTP_SERVER_ADDRESS GET "${HTTP_LOCK}" address)
  string(JSON HTTP_SERVER_PORT GET "${HTTP_LOCK}" port)
  set(HTTP_SERVER_URL "http://${HTTP_SERVER_ADDRESS}:${HTTP_SERVER_PORT}")
else ()
  message(WARNING "${TEST_OUTPUT_DIR}/vtkhttp.lock file does not exist!")
endif()

# Create user profile directory
file(MAKE_DIRECTORY "${USER_PROFILE_DIR}")

# Generate HTML for unit test
set(TEST_HTML "${USER_PROFILE_DIR}/test.html")
set(TEST_HTML_URL "file://${TEST_HTML}")
generate_index_html("${TEST_EXECUTABLE}" "${TEST_ARGS}" "${EXIT_AFTER_TEST}" "${HTTP_SERVER_URL}" "${TEST_HTML}")

message(STATUS "TEST_EXECUTABLE=${TEST_EXECUTABLE}")
message(STATUS "TEST_ARGS=${TEST_ARGS}")
message(STATUS "USER_PROFILE_DIR=${USER_PROFILE_DIR}")
message(STATUS "HTTP_SERVER_URL=${HTTP_SERVER_URL}")

set(IMPLICIT_ENGINE_ARGS "")
if (TESTING_WASM_ENGINE MATCHES "chrome|chromium|Google Chrome")
  set(IMPLICIT_ENGINE_ARGS
    "--disable-application-cache"
    "--disable-extensions"
    "--disable-notifications"
    "--disable-restore-session-state"
    "--new-window"
    "--no-default-browser-check"
    "--no-first-run"
    "--enable-logging=stderr"
    "--v=INFO:CONSOLE"
    "--user-data-dir=${USER_PROFILE_DIR}"
    "--enable-features=WebAssemblyExperimentalJSPI")
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND IMPLICIT_ENGINE_ARGS "--enable-features=Vulkan")
    list(APPEND IMPLICIT_ENGINE_ARGS "--enable-unsafe-webgpu")
    list(APPEND IMPLICIT_ENGINE_ARGS "--use-angle=Vulkan")
  endif()
endif()

if (IS_EXECUTABLE "${TESTING_WASM_ENGINE}")
  execute_process(
    COMMAND ${TESTING_WASM_ENGINE} ${IMPLICIT_ENGINE_ARGS} ${TESTING_WASM_ENGINE_ARGS} "--allow-file-access-from-files" ${TEST_HTML_URL}
    OUTPUT_VARIABLE ENGINE_ERROR_OUTPUT
    ERROR_VARIABLE ENGINE_ERROR_OUTPUT
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
    COMMAND_ECHO STDOUT
  )

  set(EXIT_CODE 1)
  if (ENGINE_ERROR_OUTPUT)
    extract_vtk_wasm_test_exit_code(${ENGINE_ERROR_OUTPUT} EXIT_CODE)
  endif()
else ()
  message(FATAL_ERROR "TESTING_WASM_ENGINE: ${TESTING_WASM_ENGINE} is not an executable!")
  set(EXIT_CODE 1)
endif ()

# Cleanup user profile directory
file(REMOVE_RECURSE "${USER_PROFILE_DIR}")

cmake_language(EXIT ${EXIT_CODE})
