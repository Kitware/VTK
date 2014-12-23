# Allow the user to customize their build with some local options
include(${VTK_SOURCE_DIR}/Wrapping/Tools/LocalUserOptions.cmake OPTIONAL)

# vtkParse.tab.c has #line markers that break #include with double quotes.
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${VTK_BINARY_DIR}/Common/Core
  ${VTK_SOURCE_DIR}/Common/Core
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

# get absolute path of each header for install rules
set(VTK_PARSE_HEADERS)
foreach(arg ${VTK_PARSE_SOURCES})
  get_filename_component(src "${arg}" ABSOLUTE)
  string(REGEX REPLACE "\\.c$" ".h" hdr "${src}")
  if("${hdr}" MATCHES "\\.h$" AND EXISTS "${hdr}")
    list(APPEND VTK_PARSE_HEADERS "${hdr}")
  endif()
endforeach()

# Create a module, but the wrapper tools module is atypical so a lot of
# customization is done here.
set(${vtk-module}_LIBRARIES ${vtk-module})
vtk_module_impl()

# Wrapper tools must be statically linked
vtk_add_library(${vtk-module} STATIC ${VTK_PARSE_SOURCES})
vtk_module_export_info()

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${VTK_PARSE_HEADERS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development
    )
endif()

set_property(SOURCE vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
  VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

if(NOT CMAKE_CROSSCOMPILING)

  add_executable(vtkWrapHierarchy vtkWrapHierarchy.c)
  target_link_libraries(vtkWrapHierarchy vtkWrappingTools)
  vtk_compile_tools_target(vtkWrapHierarchy)

  add_executable(vtkWrapTcl vtkWrapTcl.c)
  target_link_libraries(vtkWrapTcl vtkWrappingTools)
  add_executable(vtkWrapTclInit vtkWrapTclInit.c)
  vtk_compile_tools_target(vtkWrapTcl)
  vtk_compile_tools_target(vtkWrapTclInit)

  add_executable(vtkWrapPython
    vtkWrapPython.c
    vtkWrapPythonClass.c
    vtkWrapPythonConstant.c
    vtkWrapPythonEnum.c
    vtkWrapPythonMethod.c
    vtkWrapPythonMethodDef.c
    vtkWrapPythonNamespace.c
    vtkWrapPythonOverload.c
    vtkWrapPythonTemplate.c
    vtkWrapPythonType.c
  )
  target_link_libraries(vtkWrapPython vtkWrappingTools)
  add_executable(vtkWrapPythonInit vtkWrapPythonInit.c)
  vtk_compile_tools_target(vtkWrapPython)
  vtk_compile_tools_target(vtkWrapPythonInit)

  add_executable(vtkParseJava vtkParseJava.c)
  target_link_libraries(vtkParseJava vtkWrappingTools)
  add_executable(vtkWrapJava vtkWrapJava.c)
  target_link_libraries(vtkWrapJava vtkWrappingTools)
  vtk_compile_tools_target(vtkParseJava)
  vtk_compile_tools_target(vtkWrapJava)

endif()
