set(Module_SRCS
  vtkLODActor.cxx
  vtkQuadricLODActor.cxx)

vtk_module_library(vtkRenderingLOD ${Module_SRCS})
