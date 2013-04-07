project(JSONCPP)

set(vtkjsoncpp_THIRD_PARTY 1)
set(vtkjsoncpp_LIBRARIES vtkjsoncpp)
vtk_module_export_info()

set(JSONCPP_SOURCES
  jsoncpp.cpp
)

include_directories(${JSONCPP_SOURCE_DIR} ${JSONCPP_BINARY_DIR})

if (BUILD_SHARED_LIBS AND WIN32)
  add_definitions(-DJSON_DLL_BUILD)
endif()
vtk_add_library(vtkjsoncpp ${JSONCPP_SOURCES})

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY
    ${JSONCPP_SOURCE_DIR}/json
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/JSONCPP
    COMPONENT Development)
endif()
