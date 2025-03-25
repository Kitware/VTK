# Allow the user to customize their build with some local options
include("${CMAKE_CURRENT_SOURCE_DIR}/LocalUserOptions.cmake" OPTIONAL)

set(ns "VTK")
if (VTK_BUILD_COMPILE_TOOLS_ONLY)
  set(ns "VTKCompileTools")
endif ()

set(sources
  vtkParse.tab.c
  vtkParseData.c
  vtkParseDepends.c
  vtkParseExtras.c
  vtkParseHierarchy.c
  vtkParseMain.c
  vtkParseMangle.c
  vtkParseMerge.c
  vtkParseMethodType.c
  vtkParsePreprocess.c
  vtkParseProperties.c
  vtkParseString.c
  vtkParseSystem.c
  vtkWrap.c
  vtkWrapText.c)

set(headers
  vtkParse.h
  vtkParseAttributes.h
  vtkParseData.h
  vtkParseDepends.h
  vtkParseExtras.h
  vtkParseHierarchy.h
  vtkParseMain.h
  vtkParseMangle.h
  vtkParseMerge.h
  vtkParseMethodType.h
  vtkParsePreprocess.h
  vtkParseProperties.h
  vtkParseString.h
  vtkParseSystem.h
  vtkParseType.h
  vtkWrap.h
  vtkWrapText.h)

set_property(SOURCE vtkParseMain.c
  PROPERTY
    COMPILE_DEFINITIONS "VTK_PARSE_VERSION=\"${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}\"")

# Disable abi mangling in this module
set(_vtk_module_no_namespace_abi_mangling ON)

vtk_module_add_module("${ns}::WrappingTools"
  SOURCES ${sources}
  HEADERS ${headers})
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  vtk_module_compile_options("${ns}::WrappingTools"
    PRIVATE
      "-pthread")
endif ()

if (NOT TARGET "${ns}::WrapHierarchy")
  vtk_module_add_executable(WrapHierarchy
    DEVELOPMENT
    BASENAME vtkWrapHierarchy
    vtkWrapHierarchy.c)
  target_link_libraries(WrapHierarchy
    PRIVATE
      "${ns}::WrappingTools")
  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_options(WrapHierarchy
      PRIVATE
        "-pthread")
    # allows vtkWrapHierarchy to access host file system for headers, generated sources, etc.
    # permit memory growth to generate arbitrary sized hierarchy files.
    target_link_options(WrapHierarchy
      PRIVATE
        "-pthread"
        # Remove after https://github.com/WebAssembly/design/issues/1271 is closed
        "-Wno-pthreads-mem-growth"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s NODERAWFS=1")
  endif ()
  add_executable("${ns}::WrapHierarchy" ALIAS
    WrapHierarchy)
endif ()

if (NOT TARGET WrapPython)
  vtk_module_add_executable(WrapPython
    DEVELOPMENT
    BASENAME vtkWrapPython
    vtkWrapPython.c
    vtkWrapPythonClass.c
    vtkWrapPythonConstant.c
    vtkWrapPythonEnum.c
    vtkWrapPythonMethod.c
    vtkWrapPythonMethodDef.c
    vtkWrapPythonNamespace.c
    vtkWrapPythonNumberProtocol.c
    vtkWrapPythonOverload.c
    vtkWrapPythonProperty.c
    vtkWrapPythonTemplate.c
    vtkWrapPythonType.c)
  target_link_libraries(WrapPython
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapPython" ALIAS
    WrapPython)

  vtk_module_add_executable(WrapPythonInit
    DEVELOPMENT
    BASENAME vtkWrapPythonInit
    vtkWrapPythonInit.c)
  target_link_libraries(WrapPythonInit
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapPythonInit" ALIAS
    WrapPythonInit)
endif ()

if (NOT TARGET WrapJava)
  vtk_module_add_executable(ParseJava
    DEVELOPMENT
    BASENAME vtkParseJava
    vtkParseJava.c)
  target_link_libraries(ParseJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::ParseJava" ALIAS
    ParseJava)

  vtk_module_add_executable(WrapJava
    DEVELOPMENT
    BASENAME vtkWrapJava
    vtkWrapJava.c)
  target_link_libraries(WrapJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapJava" ALIAS
    WrapJava)
endif ()

if (NOT TARGET WrapSerDes)
  vtk_module_add_executable(WrapSerDes
    DEVELOPMENT
    BASENAME vtkWrapSerDes
    vtkWrapSerDes.c
    vtkWrapSerDesClass.c
    vtkWrapSerDesFunction.c
    vtkWrapSerDesProperty.c)
  target_link_libraries(WrapSerDes
    PRIVATE
      "${ns}::WrappingTools")

  if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # allows WrapSerDes to access host file system for headers, generated sources, etc.
    # permit memory growth to generate arbitrary sized vtkClassNameSerDes.cxx files
    target_compile_options(WrapSerDes
      PRIVATE
        "-pthread")
    target_link_options(WrapSerDes
      PRIVATE
        "-pthread"
        # Remove after https://github.com/WebAssembly/design/issues/1271 is closed
        "-Wno-pthreads-mem-growth"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s NODERAWFS=1")
  endif ()
  add_executable("${ns}::WrapSerDes" ALIAS
    WrapSerDes)
endif ()
