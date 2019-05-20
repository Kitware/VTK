# Allow the user to customize their build with some local options
include("${CMAKE_CURRENT_SOURCE_DIR}/LocalUserOptions.cmake" OPTIONAL)

set(ns "VTK")
if (VTK_BUILD_COMPILE_TOOLS_ONLY)
  set(ns "VTKCompileTools")
endif ()

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

vtk_module_add_module("${ns}::WrappingTools"
  SOURCES ${sources}
  HEADERS ${headers}
  FORCE_STATIC)
# This is a hard-coded path because `vtkType.h` from `VTK::CommonCore` is
# used for VTK's integer types. However, it is not a public dependency. In
# order for the compile tools project to be as minimal as possible, the
# `VTK::CommonCore` target should also not be created.
vtk_module_include("${ns}::WrappingTools"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Common/Core"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/KWIML")

set(vtk_wrapping_tools_no_install)
if (NOT _vtk_build_INSTALL_HEADERS)
  set(vtk_wrapping_tools_no_install
    NO_INSTALL)
endif ()

if (NOT TARGET "${ns}::WrapHierarchy")
  vtk_module_add_executable(WrapHierarchy
    ${vtk_wrapping_tools_no_install}
    BASENAME vtkWrapHierarchy
    vtkWrapHierarchy.c)
  target_link_libraries(WrapHierarchy
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapHierarchy" ALIAS
    WrapHierarchy)
endif ()

if (NOT TARGET WrapPython)
  vtk_module_add_executable(WrapPython
    ${vtk_wrapping_tools_no_install}
    BASENAME vtkWrapPython
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
  target_link_libraries(WrapPython
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapPython" ALIAS
    WrapPython)

  vtk_module_add_executable(WrapPythonInit
    ${vtk_wrapping_tools_no_install}
    BASENAME vtkWrapPythonInit
    vtkWrapPythonInit.c)
  add_executable("${ns}::WrapPythonInit" ALIAS
    WrapPythonInit)
endif ()

if (NOT TARGET WrapJava)
  vtk_module_add_executable(ParseJava
    ${vtk_wrapping_tools_no_install}
    BASENAME vtkParseJava
    vtkParseJava.c)
  target_link_libraries(ParseJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::ParseJava" ALIAS
    ParseJava)

  vtk_module_add_executable(WrapJava
    ${vtk_wrapping_tools_no_install}
    BASENAME vtkWrapJava
    vtkWrapJava.c)
  target_link_libraries(WrapJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapJava" ALIAS
    WrapJava)
endif ()
