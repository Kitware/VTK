project(JsonCpp)

set(vtkjsoncpp_THIRD_PARTY 1)
set(vtkjsoncpp_LIBRARIES vtkjsoncpp)
vtk_module_export_info()

if(VTK_REQUIRE_LARGE_FILE_SUPPORT)
  add_definitions(
    -D_LARGEFILE_SOURCE
    -D_LARGEFILE64_SOURCE
    -D_LARGE_FILES
    -D_FILE_OFFSET_BITS=64
    )
endif()

set(JSONCPP_SOURCES
  json/json.h
  jsoncpp.cpp
)

include_directories(${JsonCpp_SOURCE_DIR})

vtk_add_library(vtkjsoncpp ${JSONCPP_SOURCES})
set_target_properties(vtkjsoncpp
  PROPERTIES
    DEFINE_SYMBOL JSON_DLL_BUILD)

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY
    ${JsonCpp_SOURCE_DIR}/json
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkjsoncpp
    COMPONENT Development)
endif()
