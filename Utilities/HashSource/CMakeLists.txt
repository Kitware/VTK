# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#

# This executable is only run from the build tree so we do not need a
# launcher for it and should always use the RPATH to find shared libs.
SET(CMAKE_SKIP_RPATH 0)

IF(NOT CMAKE_CROSSCOMPILING)
  ADD_EXECUTABLE(vtkHashSource vtkHashSource.cxx)
  TARGET_LINK_LIBRARIES(vtkHashSource vtksys)
  EXPORT(TARGETS vtkHashSource FILE ${EXPORT_EXECUTABLES_FILE}
    NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
  IF(COMPILE_TOOLS_TARGET)
    ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkHashSource)
  ENDIF(COMPILE_TOOLS_TARGET)
ENDIF(NOT CMAKE_CROSSCOMPILING)
