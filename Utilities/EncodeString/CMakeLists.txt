# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#

IF(NOT CMAKE_CROSSCOMPILING)
  VTK_ADD_EXECUTABLE(vtkEncodeString vtkEncodeString.cxx)
  EXPORT(TARGETS vtkEncodeString FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND) 
  IF(COMPILE_TOOLS_TARGET)
    ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkEncodeString)
  ENDIF(COMPILE_TOOLS_TARGET)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(TARGETS vtkEncodeString
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF(NOT CMAKE_CROSSCOMPILING)

SET(ENCODESTRING_EXE vtkEncodeString)
