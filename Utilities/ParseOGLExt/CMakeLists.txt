# Builds a simple program that will take a text file and generate a C
# header file containing a string holding the contents of that file.  This
# provides a simple means of embedding that file into a library or
# executable.
#
# Program:   Visualization Toolkit
# Module:    CMakeLists.txt
#
# Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#
# Copyright 2003 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the
# U.S. Government. Redistribution and use in source and binary forms, with
# or without modification, are permitted provided that this Notice and any
# statement of authorship are reproduced on all copies.

IF(NOT CMAKE_CROSSCOMPILING)
  ADD_EXECUTABLE(vtkParseOGLExt Tokenizer.cxx ParseOGLExt.cxx)
  EXPORT(TARGETS vtkParseOGLExt FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND) 
  IF(COMPILE_TOOLS_TARGET)
    ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkParseOGLExt)
  ENDIF(COMPILE_TOOLS_TARGET)
ENDIF(NOT CMAKE_CROSSCOMPILING)

IF(CMAKE_CROSSCOMPILING)
  SET(PARSEOGLEXT_EXE vtkParseOGLExt)
ELSE(CMAKE_CROSSCOMPILING)
  GET_TARGET_PROPERTY(PARSEOGLEXT_EXE vtkParseOGLExt LOCATION)
ENDIF(CMAKE_CROSSCOMPILING)
SET(VTK_PARSEOGLEXT_EXE ${PARSEOGLEXT_EXE}
  CACHE INTERNAL "OpenGL extensions parser." FORCE)

SET(VTK_GLEXT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/headers/glext.h" CACHE FILEPATH 
  "Location of the OpenGL extensions header file (glext.h).")
SET(VTK_GLXEXT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/headers/glxext.h" CACHE FILEPATH
  "Location of the GLX extensions header file (glxext.h).")
SET(VTK_WGLEXT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/headers/wglext.h" CACHE FILEPATH
  "Location of the WGL extensions header file (wglext.h).")
MARK_AS_ADVANCED(VTK_GLEXT_FILE VTK_GLXEXT_FILE VTK_WGLEXT_FILE)
