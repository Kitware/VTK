set(Module_SRCS
  vtkGL2PSContextDevice2D.cxx
  vtkGL2PSUtilities.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
