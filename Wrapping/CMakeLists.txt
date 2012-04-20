# Allow the user to customize their build with some local options
#
include(${VTK_SOURCE_DIR}/Wrapping/LocalUserOptions.cmake OPTIONAL)

# vtkParse.tab.c has #line markers that break #include with double quotes.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Python
  ${VTK_SOURCE_DIR}/Common/Core
  ${VTK_BINARY_DIR}/Common/Core
  ${VTK_BINARY_DIR}/Wrapping
  ${VTK_BINARY_DIR}/Utilities/KWSys)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h)

# Get our lists of modules to be wrapped, create a list of uniques that need to
# have wrap hierarchy run on them too.
get_property(_python_modules GLOBAL PROPERTY VTK_PYTHON_WRAPPED)
get_property(_java_modules GLOBAL PROPERTY VTK_JAVA_WRAPPED)

set_property(SOURCE vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
  VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

if(NOT CMAKE_CROSSCOMPILING)
  if(VTK_WRAP_TCL OR VTK_WRAP_PYTHON OR VTK_WRAP_JAVA)
    add_executable(vtkWrapHierarchy vtkParse.tab.c vtkParsePreprocess.c
                       vtkParseExtras.c vtkWrapHierarchy.c)
    export(TARGETS vtkWrapHierarchy
      FILE ${EXPORT_EXECUTABLES_FILE}
      NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapHierarchy)
    endif()

    if(NOT VTK_INSTALL_NO_DEVELOPMENT)
    #  INSTALL(TARGETS vtkWrapHierarchy
    #    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT Development)
    endif()
  endif()

  if(VTK_WRAP_TCL)
    add_executable(vtkWrapTcl vtkWrapTcl.c vtkParseMain.c vtkParse.tab.c
                       vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    add_executable(vtkWrapTclInit vtkWrapTclInit.c)
    export(TARGETS vtkWrapTcl vtkWrapTclInit
      FILE ${EXPORT_EXECUTABLES_FILE}
      NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapTcl vtkWrapTclInit)
    endif()
  endif()

  if(VTK_WRAP_PYTHON)
    set(VTK_WRAP_PYTHON_FIND_LIBS ON)
    include(vtkWrapPython)
    include_directories("${PYTHON_INCLUDE_PATH}"
      "${CMAKE_CURRENT_SOURCE_DIR}/PythonCore"
      "${CMAKE_CURRENT_BINARY_DIR}/PythonCore")

    add_executable(vtkWrapPython vtkWrapPython.c vtkWrap.c vtkWrapText.c
                   vtkParseMain.c vtkParse.tab.c vtkParsePreprocess.c
                   vtkParseExtras.c vtkParseHierarchy.c)
    add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
    target_link_libraries(vtkWrapPython ${VTK_PYTHON_LIBRARIES})
    export(TARGETS vtkWrapPython vtkWrapPythonInit
      FILE ${EXPORT_EXECUTABLES_FILE}
      NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapPython vtkWrapPythonInit)
    endif()
  endif()

  if(VTK_WRAP_JAVA)
    add_executable(vtkParseJava vtkParseJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    add_executable(vtkWrapJava vtkWrapJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    export(TARGETS vtkParseJava vtkWrapJava
      FILE ${EXPORT_EXECUTABLES_FILE}
      NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkParseJava vtkWrapJava)
    endif()
  endif()

endif()

if(VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif()

if(VTK_WRAP_TCL)
  add_subdirectory(Tcl)
endif()
