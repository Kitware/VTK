# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl.c vtkParse.tab)
  INSTALL_TARGETS(/bin vtkWrapTcl)
  ADD_EXECUTABLE(vtkWrapTclInit vtkWrapTclInit.c)
  INSTALL_TARGETS(/bin vtkWrapTclInit)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_EXECUTABLE(vtkWrapPython vtkWrapPython.c vtkParse.tab)
  INSTALL_TARGETS(/bin vtkWrapPython)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  ADD_EXECUTABLE(vtkParseJava vtkParseJava.c vtkParse.tab)
  ADD_EXECUTABLE(vtkWrapJava vtkWrapJava.c vtkParse.tab)
  INSTALL_TARGETS(/bin vtkParseJava)
  INSTALL_TARGETS(/bin vtkWrapJava)
ENDIF (VTK_WRAP_JAVA)
