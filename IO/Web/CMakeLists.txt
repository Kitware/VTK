set(Module_SRCS
  vtkDataArrayHelper.cxx
  vtkHttpDataSetWriter.cxx
  vtkHttpSceneExporter.cxx
)

vtk_module_library(
  vtkIOWeb
  ${Module_SRCS}
)
