# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

# vtkParse.tab.c has #line markers that break #include with double quotes.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF(NOT CMAKE_CROSSCOMPILING)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES vtkParse.h vtkParseType.h vtkParsePreprocess.h vtkParseMain.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
      COMPONENT Development
      )
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

  IF (VTK_WRAP_TCL OR VTK_WRAP_PYTHON OR VTK_WRAP_JAVA)
    VTK_ADD_EXECUTABLE(vtkWrapHierarchy vtkParse.tab.c vtkParsePreprocess.c
                   vtkParseExtras.c vtkWrapHierarchy.c)
    EXPORT(TARGETS vtkWrapHierarchy FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    IF(COMPILE_TOOLS_TARGET)
      ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkWrapHierarchy)
    ENDIF(COMPILE_TOOLS_TARGET)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      INSTALL(TARGETS vtkWrapHierarchy
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development)
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF (VTK_WRAP_TCL OR VTK_WRAP_PYTHON OR VTK_WRAP_JAVA)

  IF (VTK_WRAP_TCL)
    VTK_ADD_EXECUTABLE(vtkWrapTcl vtkWrapTcl.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    VTK_ADD_EXECUTABLE(vtkWrapTclInit vtkWrapTclInit.c)
    EXPORT(TARGETS vtkWrapTcl vtkWrapTclInit FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    IF(COMPILE_TOOLS_TARGET)
      ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkWrapTcl vtkWrapTclInit)
   ENDIF(COMPILE_TOOLS_TARGET)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      INSTALL(TARGETS vtkWrapTcl vtkWrapTclInit
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
        )
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF (VTK_WRAP_TCL)

  IF (VTK_WRAP_PYTHON)
    INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_PATH}")

    VTK_ADD_EXECUTABLE(vtkWrapPython vtkWrapPython.c vtkWrap.c vtkWrapText.c
                   vtkParseMain.c vtkParse.tab.c vtkParsePreprocess.c
                   vtkParseExtras.c vtkParseHierarchy.c)
    VTK_ADD_EXECUTABLE(vtkWrapPythonInit vtkWrapPythonInit.c)
    TARGET_LINK_LIBRARIES(vtkWrapPython ${VTK_PYTHON_LIBRARIES})
    EXPORT(TARGETS vtkWrapPython vtkWrapPythonInit FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    IF(COMPILE_TOOLS_TARGET)
      ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkWrapPython vtkWrapPythonInit)
    ENDIF(COMPILE_TOOLS_TARGET)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      INSTALL(TARGETS vtkWrapPython vtkWrapPythonInit
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
        )
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF (VTK_WRAP_PYTHON)

  IF (VTK_WRAP_JAVA)
    VTK_ADD_EXECUTABLE(vtkParseJava vtkParseJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    VTK_ADD_EXECUTABLE(vtkWrapJava vtkWrapJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    EXPORT(TARGETS vtkParseJava vtkWrapJava FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    IF(COMPILE_TOOLS_TARGET)
      ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkParseJava vtkWrapJava)
    ENDIF(COMPILE_TOOLS_TARGET)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      INSTALL(TARGETS vtkParseJava vtkWrapJava
        EXPORT ${VTK_INSTALL_EXPORT_NAME}
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development # .exe, .dll
        )
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF (VTK_WRAP_JAVA)

ENDIF(NOT CMAKE_CROSSCOMPILING)
