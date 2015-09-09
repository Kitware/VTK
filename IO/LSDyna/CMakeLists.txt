set(Module_SRCS
  vtkLSDynaPart.cxx
  vtkLSDynaPartCollection.cxx
  vtkLSDynaReader.cxx
  vtkLSDynaSummaryParser.cxx
  private/LSDynaFamily.cxx
  private/LSDynaMetaData.cxx
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/private/LSDynaExport.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/private/LSDynaExport.h
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/private
  ${CMAKE_CURRENT_BINARY_DIR}/private
  )

set_source_files_properties(
  vtkLSDynaPart
  vtkLSDynaPartCollection
  private/LSDynaFamily
  private/LSDynaMetaData
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

set_source_files_properties(
  private/LSDynaFamily
  private/LSDynaMetaData
  PROPERTIES
    SKIP_HEADER_INSTALL 1
  )

vtk_module_library(vtkIOLSDyna ${Module_SRCS})
