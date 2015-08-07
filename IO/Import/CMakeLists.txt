set(Module_SRCS
  vtk3DSImporter.cxx
  vtkImporter.cxx
  vtkVRMLImporter.cxx
  vtkOBJImporter.cxx
  vtkOBJImporterInternals.cxx
)

set (${vtk-module}_HDRS
  vtk3DS.h
)

set_source_files_properties(
  vtkImporter
  ABSTRACT
  )

set_source_files_properties(
  vtkOBJImporterInternals
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOImport ${Module_SRCS})
