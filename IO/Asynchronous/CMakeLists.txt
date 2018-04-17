set(Module_SRCS
  vtkThreadedImageWriter.cxx
  )

vtk_module_library(vtkIOAsynchronous ${Module_SRCS})
