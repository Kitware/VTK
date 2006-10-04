# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

# vtkParse.tab.c has #line markers that break #include with double quotes.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF (VTK_WRAP_TCL)
  ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl.c vtkParse.tab.c)
  ADD_EXECUTABLE(vtkWrapTclInit vtkWrapTclInit.c)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(TARGETS vtkWrapTcl vtkWrapTclInit
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_EXECUTABLE(vtkWrapPython vtkWrapPython.c vtkParse.tab.c)
  ADD_EXECUTABLE(vtkWrapPythonInit vtkWrapPythonInit.c)
  TARGET_LINK_LIBRARIES(vtkWrapPython ${VTK_PYTHON_LIBRARIES})
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(TARGETS vtkWrapPython vtkWrapPythonInit
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  ADD_EXECUTABLE(vtkParseJava vtkParseJava.c vtkParse.tab.c)
  ADD_EXECUTABLE(vtkWrapJava vtkWrapJava.c vtkParse.tab.c)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(TARGETS vtkParseJava vtkWrapJava
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF (VTK_WRAP_JAVA)
