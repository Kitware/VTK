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

vtk_module_library(vtkIOLSDyna ${Module_SRCS})
