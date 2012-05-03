if(NOT DEFINED VTK_BINARY_DIR)
  message(FATAL_ERROR "Invoke with -DVTK_BINARY_DIR=/path/to/VTK/build")
endif()
if(NOT DEFINED CONFIGURATION)
  message(FATAL_ERROR "Invoke with -DCONFIGURATION=<config>")
endif()
message(STATUS "Removing InstallTest directory...")
file(REMOVE_RECURSE "${VTK_BINARY_DIR}/InstallTest")
message(STATUS "Building 'install' target...")
execute_process(
  COMMAND ${CMAKE_COMMAND} --build "${VTK_BINARY_DIR}"
                           --target install
                           --config "${CONFIGURATION}"
  RESULT_VARIABLE failed
  )
if(failed)
  message(FATAL_ERROR "Installation failed: ${failed}")
else()
  file(WRITE "${VTK_BINARY_DIR}/InstallTest/InstallSucceeded.txt" "# Installation succeeded!\n")
endif()
