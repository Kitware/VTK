IF (VTK_WRAP_TCL)
ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl vtkParse.tab)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
ADD_EXECUTABLE(vtkWrapPython vtkWrapPython vtkParse.tab)
ENDIF (VTK_WRAP_PYTHON)


