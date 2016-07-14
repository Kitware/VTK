# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#

set(VTK_BUILD_FORWARDING_EXECUTABLES FALSE)
vtk_module_export_info()

IF(NOT CMAKE_CROSSCOMPILING)
  VTK_ADD_EXECUTABLE(vtkEncodeString vtkEncodeString.cxx)
  vtk_compile_tools_target(vtkEncodeString)
ENDIF()
