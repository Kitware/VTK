# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl vtkParse.tab)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_EXECUTABLE(vtkWrapPython vtkWrapPython vtkParse.tab)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  ADD_EXECUTABLE(vtkParseJava vtkParseJava vtkParse.tab)
  ADD_EXECUTABLE(vtkWrapJava vtkWrapJava vtkParse.tab)
ENDIF (VTK_WRAP_JAVA)
