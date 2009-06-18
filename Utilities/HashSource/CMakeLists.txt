# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#

IF(NOT CMAKE_CROSSCOMPILING)
  ADD_EXECUTABLE(vtkHashSource vtkHashSource.cxx)
  TARGET_LINK_LIBRARIES(vtkHashSource vtksys)
  EXPORT(TARGETS vtkHashSource FILE ${EXPORT_EXECUTABLES_FILE}
    NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
  IF(COMPILE_TOOLS_TARGET)
    ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkHashSource)
  ENDIF(COMPILE_TOOLS_TARGET)
ENDIF(NOT CMAKE_CROSSCOMPILING)

IF(CMAKE_CROSSCOMPILING)
  SET(HASHSOURCE_EXE vtkHashSource)
ELSE(CMAKE_CROSSCOMPILING)
  GET_TARGET_PROPERTY(HASHSOURCE_EXE vtkHashSource LOCATION)
ENDIF(CMAKE_CROSSCOMPILING)
SET(VTK_HASHSOURCE_EXE ${HASHSOURCE_EXE}
  CACHE INTERNAL "Source file content hasher." FORCE)
