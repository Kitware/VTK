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

vtk_module_add_module(VTK::jsoncpp
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkjsoncpp")
if (VTK_REQUIRE_LARGE_FILE_SUPPORT)
  vtk_module_definitions(VTK::jsoncpp
    PRIVATE
      _LARGEFILE_SOURCE
      _LARGEFILE64_SOURCE
      _LARGE_FILES
      _FILE_OFFSET_BITS=64)
endif ()
vtk_module_include(VTK::jsoncpp
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/json>")
vtk_module_compile_features(VTK::jsoncpp
  PUBLIC
    cxx_std_11)
vtk_module_set_property(VTK::jsoncpp
  PROPERTY  DEFINE_SYMBOL
  VALUE     JSON_DLL_BUILD)
