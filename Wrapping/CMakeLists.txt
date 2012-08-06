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
    vtk_compile_tools_target(vtkWrapHierarchy)
  endif()

  if(VTK_WRAP_TCL)
    add_executable(vtkWrapTcl vtkWrapTcl.c vtkParseMain.c vtkParse.tab.c
                       vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
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

    add_executable(vtkWrapPython vtkWrapPython.c vtkWrap.c vtkWrapText.c
                   vtkParseMain.c vtkParse.tab.c vtkParsePreprocess.c
                   vtkParseExtras.c vtkParseHierarchy.c)
    add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
    target_link_libraries(vtkWrapPython ${VTK_PYTHON_LIBRARIES})
    vtk_compile_tools_target(vtkWrapPython)
    vtk_compile_tools_target(vtkWrapPythonInit)
  endif()

  if(VTK_WRAP_JAVA)
    add_executable(vtkParseJava vtkParseJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    add_executable(vtkWrapJava vtkWrapJava.c vtkParseMain.c vtkParse.tab.c
                   vtkParsePreprocess.c vtkParseExtras.c vtkParseHierarchy.c)
    vtk_compile_tools_target(vtkParseJava)
    vtk_compile_tools_target(vtkWrapJava)
  endif()

endif()

if(VTK_WRAP_TCL)
  add_subdirectory(Tcl)
endif()
