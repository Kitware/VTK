set(Module_SRCS
  vtk3DSImporter.cxx
  vtkImporter.cxx
  vtkVRMLImporter.cxx
)

set_source_files_properties(
  vtkImporter
  ABSTRACT
  )

vtk_module_library(vtkIOImport ${Module_SRCS})
