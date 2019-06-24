vtk_module_find_package(
  PACKAGE ADIOS2
  )

set(classes
  vtkADIOS2ReaderMultiBlock
  )

set(sources
  vtkADIOS2ReaderMultiBlock.cxx
  ADIOS2DataArray.cxx
  ADIOS2Helper.cxx ADIOS2Helper.txx
  ADIOS2SchemaManager.cxx
  schema/ADIOS2Schema.cxx schema/ADIOS2Schema.txx
  schema/xml_vtk/ADIOS2xmlVTI.cxx schema/xml_vtk/ADIOS2xmlVTI.txx
  schema/xml_vtk/ADIOS2xmlVTU.cxx schema/xml_vtk/ADIOS2xmlVTU.txx
  schema/xml_vtk/ADIOS2xmlVTK.cxx
)

set(private_headers
  ADIOS2DataArray.h
  ADIOS2Types.h
  ADIOS2Helper.h
  ADIOS2SchemaManager.h
  schema/ADIOS2Schema.h
  schema/xml_vtk/ADIOS2xmlVTI.h
  schema/xml_vtk/ADIOS2xmlVTU.h
  schema/xml_vtk/ADIOS2xmlVTK.h
)

vtk_module_add_module(VTK::IOADIOS2
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)

vtk_module_link(VTK::IOADIOS2
  PRIVATE
    adios2::adios2
)

# Use this module to get coverage using
# VTK/Utilities/Maintenance/computeCodeCoverageLocallyForOneTest.sh ./IO -R ADIOS2
#if(CMAKE_COMPILER_IS_GNUCXX)
#  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#  include(CodeCoverage)
#  APPEND_COVERAGE_COMPILER_FLAGS()
#endif()
