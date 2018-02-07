set(sources
  jsoncpp.cpp)

set(headers
  json/json-forwards.h
  json/json.h
  "${CMAKE_CURRENT_BINARY_DIR}/json/vtkjsoncpp_config.h")

if (BUILD_SHARED_LIBS)
  set(JSON_DLL 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/json/vtkjsoncpp_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/json/vtkjsoncpp_config.h")

vtk_add_library(vtkjsoncpp ${sources} ${headers})

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkjsoncpp/json"
    COMPONENT Development)
endif()

if (VTK_REQUIRE_LARGE_FILE_SUPPORT)
  target_compile_definitions(vtkjsoncpp
    PRIVATE
      _LARGEFILE_SOURCE
      _LARGEFILE64_SOURCE
      _LARGE_FILES
      _FILE_OFFSET_BITS=64)
endif ()
target_include_directories(vtkjsoncpp
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/json>")
set_target_properties(vtkjsoncpp
  PROPERTIES
    DEFINE_SYMBOL JSON_DLL_BUILD)
