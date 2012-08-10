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

set(VTK_PARSE_SOURCES
  vtkParse.h
  vtkParseType.h
  vtkParse.tab.c
  vtkParseData.c
  vtkParseMain.c
  vtkParseExtras.c
  vtkParseMangle.c
  vtkParseString.c
  vtkParseHierarchy.c
  vtkParsePreprocess.c
  vtkWrap.c
  vtkWrapText.c
)

set_source_files_properties(
  vtkParse.h
  vtkParseType.h
  HEADER_FILE_ONLY
)

# Wrapper tools must be statically linked
vtk_add_library(vtkWrappingCore STATIC ${VTK_PARSE_SOURCES})

# Get our lists of modules to be wrapped, create a list of uniques that need to
# have wrap hierarchy run on them too.
get_property(_python_modules GLOBAL PROPERTY VTK_PYTHON_WRAPPED)
get_property(_java_modules GLOBAL PROPERTY VTK_JAVA_WRAPPED)

set_property(SOURCE vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
  VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

if(NOT CMAKE_CROSSCOMPILING)
  if(VTK_WRAP_TCL OR VTK_WRAP_PYTHON OR VTK_WRAP_JAVA OR VTK_GENERATE_WRAPTOOLS)
    add_executable(vtkWrapHierarchy vtkWrapHierarchy.c)
    target_link_libraries(vtkWrapHierarchy vtkWrappingCore)
    vtk_compile_tools_target(vtkWrapHierarchy)
  endif()

  if(VTK_WRAP_TCL)
    add_executable(vtkWrapTcl vtkWrapTcl.c)
    target_link_libraries(vtkWrapTcl vtkWrappingCore)
    add_executable(vtkWrapTclInit vtkWrapTclInit.c)
    vtk_compile_tools_target(vtkWrapTcl)
    vtk_compile_tools_target(vtkWrapTclInit)
  endif()

  if(VTK_WRAP_PYTHON)
    set(VTK_WRAP_PYTHON_FIND_LIBS ON)
    include(vtkWrapPython)
    include_directories("${PYTHON_INCLUDE_PATH}"
      "${CMAKE_CURRENT_SOURCE_DIR}/Python"
      "${CMAKE_CURRENT_BINARY_DIR}/Python")

    add_executable(vtkWrapPython vtkWrapPython.c)
    target_link_libraries(vtkWrapPython vtkWrappingCore)
    add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
    vtk_compile_tools_target(vtkWrapPython)
    vtk_compile_tools_target(vtkWrapPythonInit)
  endif()

  if(VTK_WRAP_JAVA)
    add_executable(vtkParseJava vtkParseJava.c)
    target_link_libraries(vtkParseJava vtkWrappingCore)
    add_executable(vtkWrapJava vtkWrapJava.c)
    target_link_libraries(vtkWrapJava vtkWrappingCore)
    vtk_compile_tools_target(vtkParseJava)
    vtk_compile_tools_target(vtkWrapJava)
  endif()

endif()

if(VTK_WRAP_TCL)
  add_subdirectory(Tcl)
endif()
