message(STATUS "Testing disabling of stack trace printing by vtkLogger")
execute_process(COMMAND ${EXECUTABLE_PATH}
  OUTPUT_VARIABLE error_output
  RESULT_VARIABLE result_var)

if (error_output MATCHES "Stack trace")
  message(FATAL_ERROR "Stack trace was written")
endif()
