# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

# vtkParse.tab.c has #line markers that break #include with double quotes.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF (VTK_WRAP_TCL)
  ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl.c vtkParse.tab.c)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkWrapTcl)
  ADD_EXECUTABLE(vtkWrapTclInit vtkWrapTclInit.c)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkWrapTclInit)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_EXECUTABLE(vtkWrapPython vtkWrapPython.c vtkParse.tab.c)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkWrapPython)
  ADD_EXECUTABLE(vtkWrapPythonInit vtkWrapPythonInit.c)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkWrapPythonInit)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  ADD_EXECUTABLE(vtkParseJava vtkParseJava.c vtkParse.tab.c)
  ADD_EXECUTABLE(vtkWrapJava vtkWrapJava.c vtkParse.tab.c)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkParseJava)
  INSTALL_TARGETS(${VTK_INSTALL_BIN_DIR} vtkWrapJava)
ENDIF (VTK_WRAP_JAVA)
