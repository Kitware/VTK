set(module_SRCS
  vtkContextView.cxx
  vtkContextInteractorStyle.cxx
  )

vtk_module_library(vtkViewsContext2D ${module_SRCS})
