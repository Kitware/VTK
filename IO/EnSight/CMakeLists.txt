set(Module_SRCS
  vtkEnSight6BinaryReader.cxx
  vtkEnSight6Reader.cxx
  vtkEnSightGoldBinaryReader.cxx
  vtkEnSightGoldReader.cxx
  vtkEnSightMasterServerReader.cxx
  vtkEnSightReader.cxx
  vtkGenericEnSightReader.cxx
)

set_source_files_properties(
  vtkEnSightReader
  ABSTRACT
  )

vtk_module_library(vtkIOEnSight ${Module_SRCS})
