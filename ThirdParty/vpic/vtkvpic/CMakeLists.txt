configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/VPICDefinition.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/VPICDefinition.h)

set(sources
  VPICDataSet.cxx
  VPICDefinition.cxx
  VPICGlobal.cxx
  VPICHeader.cxx
  VPICPart.cxx
  VPICView.cxx
  GridExchange.cxx)

set(headers
  VPICDataSet.h
  VPICGlobal.h
  VPICHeader.h
  VPICPart.h
  VPICView.h
  GridExchange.h
  "${CMAKE_CURRENT_BINARY_DIR}/VPICDefinition.h")

vtk_module_add_module(VTK::vpic
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkvpic")
vtk_module_set_property(VTK::vpic
  PROPERTY  DEFINE_SYMBOL
  VALUE     VPIC_EXPORTS)

if (VTK_USING_MPI)
  vtk_module_definitions(VTK::vpic
    PRIVATE
      USE_PARALLEL_VPIC)
endif ()
