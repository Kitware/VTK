project(vtknetcdfcpp CXX)

vtk_add_library(vtknetcdfcpp
  cxx/netcdf.cpp
  cxx/ncvalues.cpp)
set_target_properties(vtknetcdfcpp
  PROPERTIES
    VERSION     4.2.0
    OUTPUT_NAME vtknetcdf_c++)
vtk_module_load(vtknetcdf)
target_link_libraries(vtknetcdfcpp
  PUBLIC
    ${vtknetcdf_LIBRARIES})

include(GenerateExportHeader)
generate_export_header(vtknetcdfcpp
  EXPORT_MACRO_NAME VTKNETCDFCPP_EXPORT)
target_include_directories(vtknetcdfcpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

set(headers
  cxx/ncvalues.h
  cxx/netcdfcpp.h
  cxx/vtk_netcdfcpp_mangle.h)

foreach (header IN LISTS headers)
  get_filename_component(header_name "${header}" NAME)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${header}"
    "${CMAKE_CURRENT_BINARY_DIR}/${header_name}"
    COPYONLY)
endforeach ()

list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtknetcdfcpp_export.h")

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES       ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtknetcdfcpp"
    COMPONENT   Development)
endif ()
