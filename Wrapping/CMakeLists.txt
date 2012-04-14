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

set(VTK_PARSE_HEADERS
  vtkParse.h
  vtkParseData.h
  vtkParseMain.h
  vtkParseType.h
  vtkParseExtras.h
  vtkParseMangle.h
  vtkParseString.h
  vtkParsePreprocess.h
  vtkParseHierarchy.h
)

set(VTK_PARSE_SOURCES
  vtkParse.tab.c
  vtkParseData.c
  vtkParseExtras.c
  vtkParseString.c
  vtkParsePreprocess.c
  vtkParseHierarchy.c
)

# Get our lists of modules to be wrapped, create a list of uniques that need to
# have wrap hierarchy run on them too.
get_property(_python_modules GLOBAL PROPERTY VTK_PYTHON_WRAPPED)
get_property(_java_modules GLOBAL PROPERTY VTK_JAVA_WRAPPED)

set_property(SOURCE vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
  VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

if(NOT CMAKE_CROSSCOMPILING)
  if(VTK_WRAP_TCL OR VTK_WRAP_PYTHON OR VTK_WRAP_JAVA)
    add_executable(vtkWrapHierarchy vtkWrapHierarchy.c
                   ${VTK_PARSE_SOURCES})
    vtk_target(vtkWrapHierarchy)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapHierarchy)
    endif()
  endif()

  if(VTK_WRAP_TCL)
    add_executable(vtkWrapTcl vtkParseMain.c vtkWrapTcl.c
                   ${VTK_PARSE_SOURCES})
    add_executable(vtkWrapTclInit vtkWrapTclInit.c)
    vtk_target(vtkWrapTcl)
    vtk_target(vtkWrapTclInit)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapTcl vtkWrapTclInit)
    endif()
  endif()

  if(VTK_WRAP_PYTHON)
    set(VTK_WRAP_PYTHON_FIND_LIBS ON)
    include(vtkWrapPython)
    include_directories("${PYTHON_INCLUDE_PATH}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Python"
      "${CMAKE_CURRENT_BINARY_DIR}/Python")

    add_executable(vtkWrapPython vtkParseMain.c vtkWrapPython.c
                   vtkWrap.c vtkWrapText.c vtkParseMangle.c
                   ${VTK_PARSE_SOURCES})
    add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
    target_link_libraries(vtkWrapPython ${VTK_PYTHON_LIBRARIES})
    vtk_target(vtkWrapPython)
    vtk_target(vtkWrapPythonInit)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkWrapPython vtkWrapPythonInit)
    endif()
  endif()

  if(VTK_WRAP_JAVA)
    add_executable(vtkParseJava vtkParseMain.c vtkParseJava.c
                   ${VTK_PARSE_SOURCES})
    add_executable(vtkWrapJava vtkParseMain.c vtkWrapJava.c
                   ${VTK_PARSE_SOURCES})
    vtk_target(vtkParseJava)
    vtk_target(vtkWrapJava)
    if(COMPILE_TOOLS_TARGET)
      add_dependencies(${COMPILE_TOOLS_TARGET} vtkParseJava vtkWrapJava)
    endif()
  endif()

endif()

if(VTK_WRAP_TCL)
  add_subdirectory(Tcl)
endif()
