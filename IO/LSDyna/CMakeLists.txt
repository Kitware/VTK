set(Module_SRCS
  vtkLSDynaPart.cxx
  vtkLSDynaPartCollection.cxx
  vtkLSDynaReader.cxx
  vtkLSDynaSummaryParser.cxx
  LSDynaMetaData.cxx
  LSDynaFamily.cxx
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/LSDynaExport.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/LSDynaExport.h
  )

set_source_files_properties(
  vtkLSDynaPart
  vtkLSDynaPartCollection
  LSDynaFamily
  LSDynaMetaData
  PROPERTIES
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOLSDyna ${Module_SRCS})
