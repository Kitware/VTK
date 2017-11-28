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

vtk_module_library(vtkIOImport ${Module_SRCS})
