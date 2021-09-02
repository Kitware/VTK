#==========================================================================
#
#     Program: Visualization Toolkit
#
#     Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
#     All rights reserved.
#     See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==========================================================================

message(STATUS "Testing disabling of stack trace printing by vtkLogger")
execute_process(COMMAND ${EXECUTABLE_PATH}
  OUTPUT_VARIABLE error_output
  RESULT_VARIABLE result_var)

if (error_output MATCHES "Stack trace")
  message(FATAL_ERROR "Stack trace was written")
endif()
