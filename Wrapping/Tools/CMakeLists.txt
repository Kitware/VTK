# Allow the user to customize their build with some local options
include("${CMAKE_CURRENT_SOURCE_DIR}/LocalUserOptions.cmake" OPTIONAL)

set(sources
  vtkParse.tab.c
  vtkParseData.c
  vtkParseExtras.c
  vtkParseHierarchy.c
  vtkParseMain.c
  vtkParseMangle.c
  vtkParseMerge.c
  vtkParsePreprocess.c
  vtkParseString.c
  vtkWrap.c
  vtkWrapText.c)

set(headers
  vtkParse.h
  vtkParseData.h
  vtkParseExtras.h
  vtkParseHierarchy.h
  vtkParseMain.h
  vtkParseMangle.h
  vtkParseMerge.h
  vtkParsePreprocess.h
  vtkParseString.h
  vtkParseType.h
  vtkWrap.h)

set_property(SOURCE vtkParseMain.c
  PROPERTY
    COMPILE_DEFINITIONS "VTK_PARSE_VERSION=\"${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}\"")

vtk_module_add_module(VTK::WrappingTools
  SOURCES ${sources}
  HEADERS ${headers}
  FORCE_STATIC)

if (NOT TARGET VTK::WrapHierarchy)
  vtk_module_add_executable(vtkWrapHierarchy vtkWrapHierarchy.c)
  target_link_libraries(vtkWrapHierarchy
    PRIVATE
      VTK::WrappingTools)
  add_executable(VTK::WrapHierarchy ALIAS
    vtkWrapHierarchy)
endif ()

if (NOT TARGET vtkWrapPython)
  vtk_module_add_executable(vtkWrapPython
    vtkWrapPython.c
    vtkWrapPythonClass.c
    vtkWrapPythonConstant.c
    vtkWrapPythonEnum.c
    vtkWrapPythonMethod.c
    vtkWrapPythonMethodDef.c
    vtkWrapPythonNamespace.c
    vtkWrapPythonOverload.c
    vtkWrapPythonTemplate.c
    vtkWrapPythonType.c)
  target_link_libraries(vtkWrapPython
    PRIVATE
      VTK::WrappingTools)
  add_executable(VTK::WrapPython ALIAS
    vtkWrapPython)

  vtk_module_add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
  add_executable(VTK::WrapPythonInit ALIAS
    vtkWrapPythonInit)
endif ()

if (NOT TARGET vtkWrapJava)
  vtk_module_add_executable(vtkParseJava vtkParseJava.c)
  target_link_libraries(vtkParseJava
    PRIVATE
      VTK::WrappingTools)
  add_executable(VTK::ParseJava ALIAS
    vtkParseJava)

  vtk_module_add_executable(vtkWrapJava vtkWrapJava.c)
  target_link_libraries(vtkWrapJava
    PRIVATE
      VTK::WrappingTools)
  add_executable(VTK::WrapJava ALIAS
    vtkWrapJava)
endif ()
